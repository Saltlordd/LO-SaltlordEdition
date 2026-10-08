// Host integration test: compile with __ANDROID__ and LO_IMPORT_TESTING.
// Supply a private synthetic ISO made with the user's verified default.xex.
#include "install/import_game.h"
#include "os/user_paths.h"
#include <filesystem>
#include <fstream>
#include <vector>
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/file.h>
namespace fs=std::filesystem;
void require(bool value,const char* text){if(!value)throw std::runtime_error(text);}
std::vector<char> bytes(const fs::path& path){std::ifstream f(path,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
int main(int argc,char**argv){
 require(argc==4,"ISO, bad ISO, and temporary root required");
 const auto root=fs::absolute(argv[3]);fs::create_directories(root);
 fs::create_directories(root/"real/files");fs::create_directories(root/"real/cache");
 fs::create_directory_symlink(root/"real",root/"framework-alias");
 os::user_paths::InitializeAndroid(root/"framework-alias/files",root/"framework-alias/cache");
 require(os::user_paths::AndroidFilesDir()==fs::canonical(root/"real/files"),"framework files alias resolved");
 require(os::user_paths::AndroidCacheDir()==fs::canonical(root/"real/cache"),"framework cache alias resolved");
 int fd=open(argv[1],O_RDONLY);require(fd>=0,"open source");lseek(fd,123,SEEK_SET);
 const auto game=os::user_paths::AndroidFilesDir()/"game";
 const auto success=install::InstallAndroidDisc1(fd,game);
 require(success.error.empty()&&!success.cancelled&&success.discs==std::vector<int>{1},"publish one complete disc");
 require(lseek(fd,0,SEEK_CUR)==123,"borrowed fd position preserved");
 const auto original=bytes(game/"disc1/default.xex");require(original.size()==6623232,"exact executable size");
 bool existing=false;try{install::InstallAndroidDisc1(fd,game);}catch(const std::exception&){existing=true;}
 require(existing&&bytes(game/"disc1/default.xex")==original,"existing installation preserved");
 fs::create_directories(root/"outside");
 fs::create_directory_symlink(root/"outside",os::user_paths::AndroidFilesDir()/"unsafe");
 bool unsafe=false;try{install::InstallAndroidDisc1(fd,os::user_paths::AndroidFilesDir()/"unsafe/game");}catch(const std::exception& e){unsafe=std::string(e.what()).find("ancestors must not contain")!=std::string::npos;}
 require(unsafe&&fs::is_empty(root/"outside"),"child link rejected without writing outside app root");
 bool stop=false,cancelled=false;try{
  auto r=install::InstallAndroidDisc1(fd,root/"cancel",[&](uint64_t done,uint64_t,std::string_view){if(done>=65536)stop=true;},[&]{return stop;});cancelled=r.cancelled;
 }catch(const install::Error&e){cancelled=e.cancelled();}
 require(cancelled&&!fs::exists(root/"cancel/disc1"),"cancel does not publish partial disc");
 for(auto&e:fs::directory_iterator(root/"cancel"))require(e.path().filename()==".import.lock","cancel staging cleaned");
 install::SetTestDiscWriteFailure("xenon_chr.fpd","write");bool failure=false;
 try{auto r=install::InstallAndroidDisc1(fd,root/"failure");failure=!r.error.empty();}catch(const std::exception&){failure=true;}
 install::SetTestDiscWriteFailure("","");require(failure&&!fs::exists(root/"failure/disc1"),"write failure rolls back");
 fs::create_directories(root/"locked");int lock=open((root/"locked/.import.lock").c_str(),O_WRONLY|O_CREAT,0600);
 require(lock>=0&&flock(lock,LOCK_EX|LOCK_NB)==0,"hold kernel lock");bool locked=false;
 try{install::InstallAndroidDisc1(fd,root/"locked");}catch(const std::exception&){locked=true;}
 require(locked,"concurrent importer excluded");close(lock);
 const auto retry=install::InstallAndroidDisc1(fd,root/"locked");require(retry.error.empty()&&fs::exists(root/"locked/disc1/default.xex"),"persistent lock reusable after release");
 int bad=open(argv[2],O_RDONLY);require(bad>=0,"bad fixture open");bool rejected=false;
 try{install::InstallAndroidDisc1(bad,root/"wrong");}catch(const std::exception&){rejected=true;}close(bad);
 require(rejected&&!fs::exists(root/"wrong/disc1"),"wrong executable rejected before publication");
 close(fd);std::cout<<"PASS: framework alias resolved, child link rejected, descriptor import, ownership/seek, exact executable, existing preservation, cancellation cleanup, write rollback, kernel lock exclusion/reuse, fingerprint rejection\n";
}
