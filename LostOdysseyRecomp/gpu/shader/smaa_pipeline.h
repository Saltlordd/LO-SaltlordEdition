#pragma once
#include "../../../thirdparty/smaa/SMAA_source.h"
#include "../../../thirdparty/smaa/AreaTex.h"
#include "../../../thirdparty/smaa/SearchTex.h"

// SMAA 1x HIGH, upstream color-edge/weight/neighborhood passes. Display-encoded
// color and lookup data use UNORM views, with no implicit sRGB conversion.
// The HDR variant crops and blends in FP16 for the extended-gamma scene; edge
// and weight stages stay RGBA8 masks.
struct SmaaPipeline
{
    using Device = plume::RenderDevice;
    Device *device = nullptr;
    std::unique_ptr<plume::RenderPipelineLayout> layout;
    std::unique_ptr<plume::RenderShader> shaders[3];
    std::unique_ptr<plume::RenderPipeline> pipelines[3], hdrNeighborhood;
    std::unique_ptr<plume::RenderDescriptorSet> sets[3], cropSet;
    std::unique_ptr<plume::RenderSampler> point;
    std::unique_ptr<plume::RenderTexture> area, search, stages[4], hdrStages[2];
    std::unique_ptr<plume::RenderFramebuffer> fb[4], hdrFb[2];
    std::unique_ptr<plume::RenderBuffer> areaUpload, searchUpload;
    uint32_t width=0, height=0, hdrWidth=0, hdrHeight=0;
    bool uploaded=false, vulkan=false;

    bool Init(Device *d, plume::RenderShader *vs, plume::RenderSampler *linear, bool useVulkan=false)
    {
        using namespace plume;
        device=d;vulkan=useVulkan;
        RenderDescriptorSetBuilder set;
        set.begin(); for(int i=0;i<3;++i) set.addTexture(i);
        set.addSampler(vulkan?3:0);set.addSampler(vulkan?4:1);set.end();
        RenderPipelineLayoutBuilder lb;
        lb.begin(false,false);lb.addPushConstant(0,0,16,RenderShaderStageFlag::PIXEL);
        lb.addDescriptorSet(set);lb.end();layout=lb.create(d);
        RenderSamplerDesc sd;
        sd.addressU=sd.addressV=sd.addressW=RenderTextureAddressMode::CLAMP;
        sd.minFilter=sd.magFilter=RenderFilter::NEAREST;point=d->createSampler(sd);
        std::string hlsl=R"(
#define SMAA_HLSL_4 1
#define SMAA_PRESET_HIGH 1
#ifdef __spirv__
struct SmaaParameters { float4 metrics; };
[[vk::push_constant]] ConstantBuffer<SmaaParameters> parameters;
#define metrics parameters.metrics
#else
cbuffer Metrics : register(b0) { float4 metrics; };
#endif
#define SMAA_RT_METRICS metrics
)";
        std::string smaa=smaaSource;
        if(vulkan) {
            const auto linearPos=smaa.find("SamplerState LinearSampler");
            smaa.insert(linearPos,"[[vk::binding(3,0)]] ");
            const auto pointPos=smaa.find("SamplerState PointSampler");
            smaa.insert(pointPos,"[[vk::binding(4,0)]] ");
        }
        hlsl+=smaa;
        hlsl+=R"(
Texture2D input0 : register(t0);
Texture2D input1 : register(t1);
Texture2D input2 : register(t2);
float4 edge(float4 pos : SV_Position) : SV_Target {
 float2 uv=pos.xy*metrics.xy;float4 offsets[3];SMAAEdgeDetectionVS(uv,offsets);
 return float4(SMAAColorEdgeDetectionPS(uv,offsets,input0),0,0);
}
float4 weight(float4 pos : SV_Position) : SV_Target {
 float2 uv=pos.xy*metrics.xy,pix;float4 offsets[3];SMAABlendingWeightCalculationVS(uv,pix,offsets);
 return SMAABlendingWeightCalculationPS(uv,pix,offsets,input0,input1,input2,0);
}
float4 neighborhood(float4 pos : SV_Position) : SV_Target {
 float2 uv=pos.xy*metrics.xy;float4 offset;SMAANeighborhoodBlendingVS(uv,offset);
 return float4(SMAANeighborhoodBlendingPS(uv,offset,input0,input1).rgb,1);
}
)";
        const char *entry[]={"edge","weight","neighborhood"};
        for(int i=0;i<3;++i) {
            auto c=xenos::CompileCachedHlsl(hlsl,entry[i],"ps_6_0",vulkan?xenos::ShaderBinaryFormat::Spirv:xenos::ShaderBinaryFormat::Dxil);
            if(!c.ok) { LOG_WARNING("SMAA {}: {}",entry[i],c.errors);return false; }
            shaders[i]=d->createShader(c.bytecode.data(),c.bytecode.size(),entry[i],vulkan?RenderShaderFormat::SPIRV:RenderShaderFormat::DXIL);
            RenderGraphicsPipelineDesc pd;
            pd.pipelineLayout=layout.get();pd.vertexShader=vs;pd.pixelShader=shaders[i].get();
            pd.renderTargetCount=1;pd.renderTargetFormat[0]=RenderFormat::R8G8B8A8_UNORM;
            pd.renderTargetBlend[0]=RenderBlendDesc::Copy();pd.cullMode=RenderCullMode::NONE;
            pipelines[i]=d->createGraphicsPipeline(pd);if(!pipelines[i]) return false;
            if(i==2) {
                pd.renderTargetFormat[0]=RenderFormat::R16G16B16A16_FLOAT;
                hdrNeighborhood=d->createGraphicsPipeline(pd);if(!hdrNeighborhood) return false;
            }
            sets[i]=set.create(d);sets[i]->setSampler(3,linear);sets[i]->setSampler(4,point.get());
        }
        RenderDescriptorSetBuilder crop;
        // Same set as Presentation's layout, including its display gamma ramp buffer.
        crop.begin();crop.addTexture(0);crop.addSampler(vulkan?1:0);crop.addTexture(vulkan?2:1);
        crop.addConstantBuffer(vulkan?3:1);crop.end();
        cropSet=crop.create(d);cropSet->setSampler(1,linear);
        auto lookup=[&](uint32_t w,uint32_t h,uint32_t bpp,RenderFormat format,const unsigned char *data,
            std::unique_ptr<RenderTexture> &tex,std::unique_ptr<RenderBuffer> &upload) {
            tex=d->createTexture(RenderTextureDesc::Texture2D(w,h,1,format));
            uint32_t pitch=(w*bpp+255)&~255u;
            upload=d->createBuffer(RenderBufferDesc::UploadBuffer(pitch*h));
            auto *mapped=static_cast<unsigned char *>(upload->map());
            for(uint32_t y=0;y<h;++y) memcpy(mapped+y*pitch,data+y*w*bpp,w*bpp);
            upload->unmap();
        };
        lookup(AREATEX_WIDTH,AREATEX_HEIGHT,2,RenderFormat::R8G8_UNORM,areaTexBytes,area,areaUpload);
        lookup(SEARCHTEX_WIDTH,SEARCHTEX_HEIGHT,1,RenderFormat::R8_UNORM,searchTexBytes,search,searchUpload);
        return true;
    }
    // hdr: the source is an extended-gamma FP16 scene and cropPipeline renders
    // to FP16; the returned texture is FP16. Otherwise unchanged RGBA8 SDR.
    plume::RenderTexture *Draw(plume::RenderCommandList *c,plume::RenderTexture *source,uint32_t w,uint32_t h,
        plume::RenderPipelineLayout *cropLayout,plume::RenderPipeline *cropPipeline,bool hdr,
        plume::RenderBuffer *ramp,uint64_t rampBytes)
    {
        using namespace plume;
        // Owner must wait its presentation fence between calls, including resize.
        if(!uploaded) {
            auto upload=[&](RenderTexture *tex,RenderBuffer *buf,RenderFormat fmt,uint32_t tw,uint32_t th,uint32_t bpp) {
                c->barriers(RenderBarrierStage::COPY,RenderTextureBarrier(tex,RenderTextureLayout::COPY_DEST));
                c->copyTextureRegion(RenderTextureCopyLocation::Subresource(tex),
                    RenderTextureCopyLocation::PlacedFootprint(buf,fmt,tw,th,1,((tw*bpp+255)&~255u)/bpp));
                c->barriers(RenderBarrierStage::GRAPHICS,RenderTextureBarrier(tex,RenderTextureLayout::SHADER_READ));
            };
            upload(area.get(),areaUpload.get(),RenderFormat::R8G8_UNORM,AREATEX_WIDTH,AREATEX_HEIGHT,2);
            upload(search.get(),searchUpload.get(),RenderFormat::R8_UNORM,SEARCHTEX_WIDTH,SEARCHTEX_HEIGHT,1);
            uploaded=true;
        }
        if(width!=w || height!=h) {
            for(int i=0;i<4;++i) {
                fb[i].reset();stages[i]=device->createTexture(RenderTextureDesc::Texture2D(w,h,1,RenderFormat::R8G8B8A8_UNORM,RenderTextureFlag::RENDER_TARGET));
                const RenderTexture *attachment[]={stages[i].get()};fb[i]=device->createFramebuffer(RenderFramebufferDesc(attachment,1));
            }
            width=w;height=h;
        }
        if(hdr && (hdrWidth!=w || hdrHeight!=h)) {
            for(int i=0;i<2;++i) {
                hdrFb[i].reset();hdrStages[i]=device->createTexture(RenderTextureDesc::Texture2D(w,h,1,RenderFormat::R16G16B16A16_FLOAT,RenderTextureFlag::RENDER_TARGET));
                const RenderTexture *attachment[]={hdrStages[i].get()};hdrFb[i]=device->createFramebuffer(RenderFramebufferDesc(attachment,1));
            }
            hdrWidth=w;hdrHeight=h;
        }
        RenderTexture *cropped=hdr?hdrStages[0].get():stages[0].get();
        RenderTexture *output=hdr?hdrStages[1].get():stages[3].get();
        RenderFramebuffer *croppedFb=hdr?hdrFb[0].get():fb[0].get();
        RenderFramebuffer *outputFb=hdr?hdrFb[1].get():fb[3].get();
        RenderViewport vp(0,0,float(w),float(h));RenderRect rect(0,0,w,h);
        c->setViewports(&vp,1);c->setScissors(&rect,1);
        // Native-size crop removes padded storage before neighborhood searches.
        c->barriers(RenderBarrierStage::GRAPHICS,RenderTextureBarrier(source,RenderTextureLayout::SHADER_READ));
        c->barriers(RenderBarrierStage::GRAPHICS,RenderTextureBarrier(cropped,RenderTextureLayout::COLOR_WRITE));
        c->setFramebuffer(croppedFb);cropSet->setTexture(0,source,RenderTextureLayout::SHADER_READ);
        cropSet->setTexture(2,source,RenderTextureLayout::SHADER_READ);
        cropSet->setBuffer(3,ramp,rampBytes);
        // Match Presentation's complete 64-byte push-constant range. Cropping
        // keeps the source encoding; the final pass owns output conversion.
        struct { float x,y,w,h,sw,sh;uint32_t aa,filter,expandRange,outputFlags;float outputScale,peakRatio;float calibrationRect[4]; }
            crop{0,0,float(w),float(h),float(w),float(h),0,0,0,0,1,1};
        c->setGraphicsPipelineLayout(cropLayout);c->setPipeline(cropPipeline);
        c->setGraphicsPushConstants(0,&crop);c->setGraphicsDescriptorSet(cropSet.get(),0);c->drawInstanced(3,1,0,0);
        // Initialize every descriptor even where an entry point does not consume it.
        for(int i=0;i<3;++i) {
            sets[i]->setTexture(0,i==1?stages[1].get():cropped,RenderTextureLayout::SHADER_READ);
            sets[i]->setTexture(1,i==2?stages[2].get():area.get(),RenderTextureLayout::SHADER_READ);
            sets[i]->setTexture(2,search.get(),RenderTextureLayout::SHADER_READ);
        }
        float metrics[]={1.0f/w,1.0f/h,float(w),float(h)};
        for(int i=0;i<3;++i) {
            RenderTexture *input=i==0?cropped:stages[i].get();
            RenderTexture *target=i==2?output:stages[i+1].get();
            c->barriers(RenderBarrierStage::GRAPHICS,RenderTextureBarrier(input,RenderTextureLayout::SHADER_READ));
            c->barriers(RenderBarrierStage::GRAPHICS,RenderTextureBarrier(target,RenderTextureLayout::COLOR_WRITE));
            // Edge detection discards edge-free pixels and needs the clear; the
            // weight and blend passes write every pixel of the full-size viewport.
            c->setFramebuffer(i==2?outputFb:fb[i+1].get());if(i==0) c->clearColor(0,RenderColor(0,0,0,0));
            c->setGraphicsPipelineLayout(layout.get());c->setPipeline(i==2&&hdr?hdrNeighborhood.get():pipelines[i].get());
            c->setGraphicsPushConstants(0,metrics);c->setGraphicsDescriptorSet(sets[i].get(),0);c->drawInstanced(3,1,0,0);
        }
        return output;
    }
};
