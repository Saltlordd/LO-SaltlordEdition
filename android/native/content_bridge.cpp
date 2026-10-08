#include <install/import_game.h>
#include "content_bridge.h"
#include <kernel/io/disc_set.h>
#include <os/user_paths.h>
#include <os/logger.h>
#include <jni.h>
#include <chrono>
#include <fstream>
#include <mutex>
#include <algorithm>

namespace {
std::mutex contentMutex;
void RequireValid(const install::ContentScan& scan) {
    if(!scan.rejected.empty())throw install::Error(scan.rejected.front().second);
    if(scan.discs.empty()&&scan.packages.empty())throw install::Error("No supported Lost Odyssey discs or DLC found");
}
void CheckEdition(const install::ContentScan& scan,const std::filesystem::path& game) {
    const auto current=DiscSet::ReadIdentity(game/"disc1");
    if(!current.edition)return;
    for(const auto& disc:scan.discs)
        if(disc.version!=current.edition)throw install::Error("Disc edition differs from installed Disc 1; do not mix USA/Europe and Asian discs");
}
}
extern "C" JNIEXPORT jstring JNICALL
Java_io_github_freefrank_lostodyssey_RuntimeReadinessActivity_nativeStageContent(JNIEnv* env,jclass,jstring source) {
    std::lock_guard lock(contentMutex);
    std::filesystem::path staging;
    try {
        const char* chars=env->GetStringUTFChars(source,nullptr);
        if(!chars)return nullptr;
        const std::string input(chars);env->ReleaseStringUTFChars(source,chars);
        auto files=os::user_paths::AndroidFilesDir();
        if(!files.is_absolute())throw install::Error("Game startup has not initialized storage yet; try again after boot");
        const auto pending=std::filesystem::weakly_canonical(files/"pending-content");
        const auto scan=install::ScanContent(std::filesystem::canonical(input));RequireValid(scan);
        CheckEdition(scan,files/"game");
        for(const auto& disc:scan.discs)if(disc.version!=3)throw install::Error("This Android build needs the USA/Europe disc edition. Please use four matching discs.");
        std::filesystem::create_directories(pending);
        const auto id="batch-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
        staging=pending/id;
        const auto result=install::InstallContent(scan,staging);
        if(!result.error.empty()||result.cancelled)throw install::Error(result.error.empty()?"Import cancelled":result.error);
        // Only complete upstream-validated installs get a ready marker outside the batch.
        const auto temporary=pending/(id+".tmp"),ready=pending/(id+".ready");
        {std::ofstream marker(temporary);marker<<"1\n";marker.close();if(!marker)throw install::Error("Could not mark import ready");}
        std::filesystem::rename(temporary,ready);
        const std::string message="OK:Ready to install: "+std::to_string(result.discs.size())+" disc(s) and "+std::to_string(result.dlcImported.size()+result.dlcUnchanged.size())+" DLC pack(s). Restart LO: Saltlord Edition to apply them. Your original files were kept.";
        LOG_INFO("Android content staged: discs={} DLC={} (applied next boot)",result.discs.size(),result.dlcImported.size()+result.dlcUnchanged.size());
        return env->NewStringUTF(message.c_str());
    }catch(const std::exception& e) {
        if(!staging.empty()){std::error_code ignored;std::filesystem::remove_all(staging,ignored);}
        LOG_ERROR("Android content staging failed: {}",e.what());
        return env->NewStringUTF((std::string("ERROR:")+e.what()).c_str());
    }
}
std::string ApplyPendingAndroidContent(const AndroidContentProgress& progress,const AndroidContentCancelled& cancelled) {
    std::lock_guard lock(contentMutex);
    const auto files=os::user_paths::AndroidFilesDir();
    const auto pending=std::filesystem::weakly_canonical(files/"pending-content");
    if(!std::filesystem::is_directory(pending))return {};
    std::string warnings;
    std::vector<std::filesystem::path> markers;
    for(const auto& item:std::filesystem::directory_iterator(pending))
        if(item.is_regular_file()&&item.path().extension()==".ready")markers.push_back(item.path());
    std::sort(markers.begin(),markers.end());
    for(const auto& marker:markers) {
        if(cancelled&&cancelled()){warnings+="Installation cancelled. Added files are kept for the next start.\n";break;}
        try {
        if(progress)progress(0,1,"Checking added files…");
        const auto batch=pending/marker.stem();
        const auto scan=install::ScanContent(batch);RequireValid(scan);CheckEdition(scan,files/"game");
        std::filesystem::create_directories(files/"game");
        const auto result=install::ReimportContent(scan,std::filesystem::canonical(files/"game"),progress,cancelled);
        if(!result.error.empty()||result.cancelled)throw install::Error(result.error.empty()?"Pending import cancelled":result.error);
        LOG_INFO("Android pending content applied: discs={} DLC={} unchanged={}",result.discs.size(),result.dlcImported.size(),result.dlcUnchanged.size());
        // Success is the commit point. Cleanup failures must not undo it.
        std::error_code ignored;std::filesystem::remove(marker,ignored);std::filesystem::remove_all(batch,ignored);
        } catch(const std::exception& e) {
            LOG_ERROR("Pending content import retained for retry: {}",e.what());
            warnings += std::string(e.what())+"\n";
        }
    }
    if(progress)progress(0,0,"");
    return warnings;
}

namespace {
std::vector<std::filesystem::path> ReadyBatches(const std::filesystem::path& files) {
    std::vector<std::filesystem::path> batches;
    const auto pending=files/"pending-content";
    if(std::filesystem::is_directory(pending))for(const auto& item:std::filesystem::directory_iterator(pending))
        if(item.is_regular_file()&&item.path().extension()==".ready")batches.push_back(pending/item.path().stem());
    std::sort(batches.begin(),batches.end());return batches;
}
}
extern "C" JNIEXPORT jboolean JNICALL
Java_io_github_freefrank_lostodyssey_RuntimeReadinessActivity_nativeHasDiscOne(JNIEnv*,jclass) {
    std::unique_lock lock(contentMutex,std::try_to_lock);if(!lock.owns_lock())return false;
    try {
        const auto files=os::user_paths::AndroidFilesDir();
        if(DiscSet::Validate(files/"game/disc1",{3,1}))return true;
        for(const auto& batch:ReadyBatches(files))if(DiscSet::Validate(batch/"disc1",{3,1}))return true;
    }catch(const std::exception& e){LOG_ERROR("Setup disc status: {}",e.what());}
    return false;
}
extern "C" JNIEXPORT jstring JNICALL
Java_io_github_freefrank_lostodyssey_RuntimeReadinessActivity_nativeContentStatus(JNIEnv* env,jclass) {
    std::unique_lock lock(contentMutex,std::try_to_lock);
    if(!lock.owns_lock())return env->NewStringUTF("Installation in progress…");
    try {
        const auto files=os::user_paths::AndroidFilesDir();const auto batches=ReadyBatches(files);std::string text;
        for(unsigned i=1;i<=4;i++) {
            const auto name="disc"+std::to_string(i);const bool installed=DiscSet::Validate(files/"game"/name,{3,i});bool pending=false;
            for(const auto& batch:batches)pending|=DiscSet::Validate(batch/name,{3,i});
            text+="Disc "+std::to_string(i)+": "+(pending?(installed?"update ready — restart required":"ready — install at next start"):(installed?"installed":"not added"))+"\n";
        }
        return env->NewStringUTF(text.c_str());
    }catch(const std::exception& e){return env->NewStringUTF("Unable to read disc status. Existing files are kept.");}
}
