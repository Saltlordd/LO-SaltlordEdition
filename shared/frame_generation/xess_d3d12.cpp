#include "d3d12.h"
#if defined(_WIN32) && defined(FRAMEGEN_WITH_XESS)
#include <xess_fg/xefg_swapchain.h>
#include <xess_fg/xefg_swapchain_d3d12.h>
#include <xell/xell.h>
#include <xell/xell_d3d12.h>
#include <cstdio>

namespace framegen {
namespace {
// XeSS-FG and XeLL are loaded at runtime so a build without the DLLs keeps
// ordinary presentation. decltype only names the SDK prototypes.
template<class T> T Export(HMODULE module,const char* name) { return reinterpret_cast<T>(GetProcAddress(module,name)); }
bool Check(xefg_swapchain_result_t result,const char* operation,std::string& reason) {
    // Positive values are warnings (old driver, skipped interpolation).
    if (result>=XEFG_SWAPCHAIN_RESULT_SUCCESS) return true;
    reason=std::string(operation)+" result="+std::to_string(int(result)); return false;
}
bool Check(xell_result_t result,const char* operation,std::string& reason) {
    if (result==XELL_RESULT_SUCCESS) return true;
    reason=std::string(operation)+" result="+std::to_string(int(result)); return false;
}
void SdkLog(const char* message,xefg_swapchain_logging_level_t level,void*) {
    if (level>=XEFG_SWAPCHAIN_LOGGING_LEVEL_WARNING)
        std::fprintf(stderr,"XeSS FG SDK: %s\n",message ? message : "(null)");
}
// Camera basis rows and translation reproduce the host view matrix used by
// BuildCamera: row-major, row-vector, t = -dot(position, axis).
void ViewMatrix(const Camera& c,float out[16]) {
    const auto dot=[&](const std::array<float,3>& axis) {
        return -(c.position[0]*axis[0]+c.position[1]*axis[1]+c.position[2]*axis[2]);
    };
    const float m[16]={c.right[0],c.up[0],c.forward[0],0, c.right[1],c.up[1],c.forward[1],0,
        c.right[2],c.up[2],c.forward[2],0, dot(c.right),dot(c.up),dot(c.forward),1};
    for (unsigned i=0;i<16;++i) out[i]=m[i];
}
class XessSession final : public D3D12Session {
    HMODULE fgModule_=nullptr, xellModule_=nullptr;
    decltype(&xefgSwapChainD3D12CreateContext) createContext_=nullptr;
    decltype(&xefgSwapChainSetLoggingCallback) setLogging_=nullptr;
    decltype(&xefgSwapChainSetLatencyReduction) setLatency_=nullptr;
    decltype(&xefgSwapChainGetProperties) properties_=nullptr;
    decltype(&xefgSwapChainD3D12InitFromSwapChainDesc) initFromDesc_=nullptr;
    decltype(&xefgSwapChainD3D12GetInitializationParameters) initParameters_=nullptr;
    decltype(&xefgSwapChainD3D12GetSwapChainPtr) swapchainPtr_=nullptr;
    decltype(&xefgSwapChainSetNumInterpolatedFrames) setFrames_=nullptr;
    decltype(&xefgSwapChainD3D12TagFrameResource) tagResource_=nullptr;
    decltype(&xefgSwapChainTagFrameConstants) tagConstants_=nullptr;
    decltype(&xefgSwapChainSetEnabled) setEnabled_=nullptr;
    decltype(&xefgSwapChainSetPresentId) setPresentId_=nullptr;
    decltype(&xefgSwapChainGetLastPresentStatus) presentStatus_=nullptr;
    decltype(&xefgSwapChainDestroy) destroy_=nullptr;
    decltype(&xellD3D12CreateContext) xellCreate_=nullptr;
    decltype(&xellSetSleepMode) xellSleepMode_=nullptr;
    decltype(&xellSleep) xellSleep_=nullptr;
    decltype(&xellAddMarkerData) xellMarker_=nullptr;
    decltype(&xellDestroyContext) xellDestroy_=nullptr;
    xefg_swapchain_handle_t xefg_=nullptr;
    xell_context_handle_t xell_=nullptr;
    ComPtr<IDXGISwapChain4> swapchain_;
    uint32_t frameId_=0, configuredFrames_=0;
    bool enabled_=false, marked_=false;
    bool Mark(xell_latency_marker_type_t marker,const char* operation,std::string& reason) {
        return !marked_ || Check(xellMarker_(xell_,frameId_,marker),operation,reason);
    }
    bool Load(HMODULE& module,const std::filesystem::path& path,std::string& reason) {
        std::error_code ec;
        if (!std::filesystem::is_regular_file(path,ec)) { reason="Intel runtime missing: "+path.string(); return false; }
        module=LoadLibraryExW(path.c_str(),nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
        if (!module) { reason="Intel runtime load failed: "+path.string(); return false; }
        return true;
    }
    bool ApplyFrameCount(std::string& reason) {
        if (!xefg_ || !swapchain_ || configuredFrames_==requested_.generatedFrames) return true;
        if (!Check(setFrames_(xefg_,requested_.generatedFrames),"XeSS FG interpolated frames",reason)) return false;
        configuredFrames_=requested_.generatedFrames; return true;
    }
public:
    ~XessSession() override { std::string reason; if (!Shutdown(reason)) { std::fprintf(stderr,"XeSS FG shutdown: %s\n",reason.c_str()); std::terminate(); } }
    bool Initialize(ID3D12Device* device,IDXGIFactory4* factory,const Config& config,
        const std::filesystem::path& directory,std::string& reason) {
        if (config.provider!=Provider::Xess || config.mode!=Mode::Fixed || !config.generatedFrames) {
            reason="XeSS FG requires a fixed multiplier"; return false;
        }
        if (!InitializeBase(device,factory,config,reason)) return false;
        std::error_code ec; const auto root=std::filesystem::absolute(directory,ec);
        if (ec) { reason="invalid XeSS runtime directory"; return false; }
        // XeLL must outlive XeSS-FG; load it first and unload it last.
        if (!Load(xellModule_,root/L"libxell.dll",reason) || !Load(fgModule_,root/L"libxess_fg.dll",reason)) return false;
        xellCreate_=Export<decltype(xellCreate_)>(xellModule_,"xellD3D12CreateContext");
        xellSleepMode_=Export<decltype(xellSleepMode_)>(xellModule_,"xellSetSleepMode");
        xellSleep_=Export<decltype(xellSleep_)>(xellModule_,"xellSleep");
        xellMarker_=Export<decltype(xellMarker_)>(xellModule_,"xellAddMarkerData");
        xellDestroy_=Export<decltype(xellDestroy_)>(xellModule_,"xellDestroyContext");
        createContext_=Export<decltype(createContext_)>(fgModule_,"xefgSwapChainD3D12CreateContext");
        setLogging_=Export<decltype(setLogging_)>(fgModule_,"xefgSwapChainSetLoggingCallback");
        setLatency_=Export<decltype(setLatency_)>(fgModule_,"xefgSwapChainSetLatencyReduction");
        properties_=Export<decltype(properties_)>(fgModule_,"xefgSwapChainGetProperties");
        initFromDesc_=Export<decltype(initFromDesc_)>(fgModule_,"xefgSwapChainD3D12InitFromSwapChainDesc");
        initParameters_=Export<decltype(initParameters_)>(fgModule_,"xefgSwapChainD3D12GetInitializationParameters");
        swapchainPtr_=Export<decltype(swapchainPtr_)>(fgModule_,"xefgSwapChainD3D12GetSwapChainPtr");
        setFrames_=Export<decltype(setFrames_)>(fgModule_,"xefgSwapChainSetNumInterpolatedFrames");
        tagResource_=Export<decltype(tagResource_)>(fgModule_,"xefgSwapChainD3D12TagFrameResource");
        tagConstants_=Export<decltype(tagConstants_)>(fgModule_,"xefgSwapChainTagFrameConstants");
        setEnabled_=Export<decltype(setEnabled_)>(fgModule_,"xefgSwapChainSetEnabled");
        setPresentId_=Export<decltype(setPresentId_)>(fgModule_,"xefgSwapChainSetPresentId");
        presentStatus_=Export<decltype(presentStatus_)>(fgModule_,"xefgSwapChainGetLastPresentStatus");
        destroy_=Export<decltype(destroy_)>(fgModule_,"xefgSwapChainDestroy");
        if (!xellCreate_ || !xellSleepMode_ || !xellSleep_ || !xellMarker_ || !xellDestroy_ ||
            !createContext_ || !setLogging_ || !setLatency_ || !properties_ || !initFromDesc_ || !initParameters_ ||
            !swapchainPtr_ || !setFrames_ || !tagResource_ || !tagConstants_ || !setEnabled_ || !setPresentId_ ||
            !presentStatus_ || !destroy_) { reason="XeSS FG/XeLL mandatory export absent"; return false; }
        if (!Check(xellCreate_(device,&xell_),"XeLL context",reason)) return false;
        xell_sleep_params_t sleep{}; sleep.bLowLatencyMode=1;
        if (!Check(xellSleepMode_(xell_,&sleep),"XeLL low-latency mode",reason)) return false;
        // Context creation is the SDK's adapter capability check.
        if (!Check(createContext_(device,&xefg_),"XeSS FG context",reason)) return false;
        setLogging_(xefg_,XEFG_SWAPCHAIN_LOGGING_LEVEL_WARNING,SdkLog,nullptr);
        if (!Check(setLatency_(xefg_,xell_),"XeSS FG latency reduction",reason)) return false;
        xefg_swapchain_properties_t props{};
        if (!Check(properties_(xefg_,&props),"XeSS FG properties",reason)) return false;
        caps_={props.maxSupportedInterpolations>0,props.maxSupportedInterpolations,false};
        const auto selected=Select(requested_,caps_);
        if (!selected.Enabled()) {
            reason="XeSS FG multiplier unsupported: max_generated_frames="+std::to_string(caps_.maxGeneratedFrames);
            return false;
        }
        return true;
    }
    HRESULT CreateSwapchain(ID3D12CommandQueue* queue,HWND window,const DXGI_SWAP_CHAIN_DESC1& desc,
        const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* fullscreen,IDXGISwapChain1** output) override {
        if (!output || !queue || swapchain_) return E_INVALIDARG;
        *output=nullptr;
        xefg_swapchain_d3d12_init_params_t params{};
        // BuildCamera supplies the remapped reversed-Z depth; PrepareNative
        // rejects any other convention rather than reinitializing.
        params.initFlags=XEFG_SWAPCHAIN_INIT_FLAG_INVERTED_DEPTH;
        params.maxInterpolatedFrames=XEFG_SWAPCHAIN_USE_MAX_SUPPORTED_INTERPOLATED_FRAMES;
        params.uiMode=XEFG_SWAPCHAIN_UI_MODE_NONE;
        std::string reason;
        if (!Check(initFromDesc_(xefg_,window,&desc,fullscreen,queue,factory_.Get(),&params),"XeSS FG swapchain",reason)) {
            std::fprintf(stderr,"XeSS FG: %s\n",reason.c_str()); return E_FAIL;
        }
        if (!Check(swapchainPtr_(xefg_,IID_PPV_ARGS(&swapchain_)),"XeSS FG swapchain pointer",reason) || !swapchain_) {
            std::fprintf(stderr,"XeSS FG: %s\n",reason.c_str()); return E_FAIL;
        }
        xefg_swapchain_d3d12_init_params_t applied{};
        if (Check(initParameters_(xefg_,&applied),"XeSS FG initialization parameters",reason))
            caps_.maxGeneratedFrames=applied.maxInterpolatedFrames;
        queue_=queue;
        return swapchain_->QueryInterface(IID_PPV_ARGS(output));
    }
    bool SubmitStart(std::string& reason) override {
        return D3D12Session::SubmitStart(reason) && Mark(XELL_RENDERSUBMIT_START,"XeLL render submit start",reason);
    }
    bool PresentStart(std::string& reason) override {
        if (!marked_) return true;
        if (!Mark(XELL_RENDERSUBMIT_END,"XeLL render submit end",reason) ||
            !Mark(XELL_PRESENT_START,"XeLL present start",reason)) return false;
        // Identifies the resources tagged for this frame; must precede Present.
        return Check(setPresentId_(xefg_,frameId_),"XeSS FG present ID",reason);
    }
private:
    bool PrepareNative(const D3D12Frame& f,ID3D12GraphicsCommandList* commands,bool reset,std::string& reason) override {
        if (!swapchain_) { reason="XeSS FG swapchain not created"; return false; }
        if (!f.camera.depthReversed) { reason="XeSS FG swapchain was initialized for reversed depth"; return false; }
        if (!Select(requested_,caps_).Enabled()) { reason="unsupported XeSS FG multiplier"; return false; }
        if (!ApplyFrameCount(reason)) return false;
        marked_=true;
        if (!Check(xellSleep_(xell_,frameId_),"XeLL sleep",reason) ||
            !Mark(XELL_SIMULATION_START,"XeLL simulation start",reason) ||
            !Mark(XELL_SIMULATION_END,"XeLL simulation end",reason)) return false;
        const auto tag=[&](xefg_swapchain_resource_type_t type,ID3D12Resource* resource,D3D12_RESOURCE_STATES state) {
            xefg_swapchain_d3d12_resource_data_t data{};
            data.type=type; data.validity=XEFG_SWAPCHAIN_RV_UNTIL_NEXT_PRESENT;
            data.resourceBase={0,0}; data.resourceSize={f.inputWidth,f.inputHeight};
            data.pResource=resource; data.incomingState=state;
            return tagResource_(xefg_,commands,frameId_,&data);
        };
        if (!Check(tag(XEFG_SWAPCHAIN_RES_DEPTH,f.depth,f.depthState),"XeSS FG depth tag",reason) ||
            !Check(tag(XEFG_SWAPCHAIN_RES_MOTION_VECTOR,f.motion,f.motionState),"XeSS FG motion tag",reason)) return false;
        xefg_swapchain_frame_constant_data_t constants{};
        ViewMatrix(f.camera,constants.viewMatrix);
        for (unsigned i=0;i<16;++i) constants.projectionMatrix[i]=f.camera.viewToClip[i];
        // Motion is unjittered, previous-minus-current, in input pixels.
        constants.jitterOffsetX=f.camera.jitterX; constants.jitterOffsetY=f.camera.jitterY;
        constants.motionVectorScaleX=1; constants.motionVectorScaleY=1;
        constants.resetHistory=reset ? 1u : 0u;
        constants.frameRenderTime=f.deltaMilliseconds;
        if (!Check(tagConstants_(xefg_,frameId_,&constants),"XeSS FG constants",reason)) return false;
        if (!enabled_) {
            if (!Check(setEnabled_(xefg_,1),"XeSS FG enable",reason)) return false;
            enabled_=true;
        }
        return true;
    }
    bool DisableNative(std::string& reason) override {
        if (!xefg_ || !enabled_) return true;
        if (!Check(setEnabled_(xefg_,0),"XeSS FG off",reason)) return false;
        enabled_=false; return true;
    }
    bool PresentedNative(bool accepted,std::string& reason) override {
        if (!marked_) { statistics_.RawPresent(accepted); return true; }
        if (!Mark(XELL_PRESENT_END,"XeLL present end",reason)) return false;
        marked_=false; ++frameId_;
        xefg_swapchain_present_status_t s{};
        const bool queried=presentStatus_(xefg_,&s)>=XEFG_SWAPCHAIN_RESULT_SUCCESS;
        const bool healthy=queried && s.frameGenResult>=XEFG_SWAPCHAIN_RESULT_SUCCESS;
        active_=accepted && prepared_ && healthy && s.isFrameGenEnabled;
        statistics_.Observe(queried,accepted,active_,s.framesPresented);
        if (!healthy) {
            reason=queried ? "XeSS FG runtime status="+std::to_string(int(s.frameGenResult)) : "XeSS FG present status unavailable";
            active_=false; history_.Reset();
            std::string ignored; if (!DisableNative(ignored)) { reason+="; "+ignored; return false; }
        }
        return true;
    }
    bool ShutdownNative(std::string& reason) override {
        if (!DisableNative(reason)) return false;
        swapchain_.Reset();
        // Destroy fails while any proxy swapchain reference is outstanding.
        if (xefg_) {
            if (!Check(destroy_(xefg_),"XeSS FG destroy",reason)) return false;
            xefg_=nullptr;
        }
        if (xell_) {
            if (!Check(xellDestroy_(xell_),"XeLL destroy",reason)) return false;
            xell_=nullptr;
        }
        for (HMODULE* module:{&fgModule_,&xellModule_}) {
            if (*module && !FreeLibrary(*module)) { reason="XeSS runtime unload failed"; return false; }
            *module=nullptr;
        }
        return true;
    }
};
}
std::unique_ptr<D3D12Session> CreateXessD3D12(ID3D12Device* d,IDXGIFactory4* f,const Config& c,
    const std::filesystem::path& path,std::string& reason) {
    auto out=std::make_unique<XessSession>();
    if (!out->Initialize(d,f,c,path,reason)) return {};
    return out;
}
} // namespace framegen
#endif
