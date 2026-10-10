#include "plume_android_loader.h"
#include <cassert>
#include <cstdio>
PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr=nullptr;
static unsigned systemInitializations=0;
static VkResult nextResult=VK_SUCCESS;
static PFN_vkVoidFunction VKAPI_PTR Selected(VkInstance,const char*){return nullptr;}
VkResult volkInitialize(){++systemInitializations;if(nextResult==VK_SUCCESS)vkGetInstanceProcAddr=Selected;return nextResult;}
int main(){
 // A fresh system launch still initializes exactly once.
 assert(plume::InitializeAndroidVulkanLoader()==VK_SUCCESS && systemInitializations==1);
 // Custom-selected dispatch must never be replaced by system initialization.
 vkGetInstanceProcAddr=Selected;auto chosen=vkGetInstanceProcAddr;
 assert(plume::InitializeAndroidVulkanLoader()==VK_SUCCESS);
 assert(vkGetInstanceProcAddr==chosen && systemInitializations==1);
 // An explicit cleared loader propagates failures and permits a later retry.
 vkGetInstanceProcAddr=nullptr;nextResult=VK_ERROR_INITIALIZATION_FAILED;
 assert(plume::InitializeAndroidVulkanLoader()==nextResult && systemInitializations==2);
 nextResult=VK_SUCCESS;assert(plume::InitializeAndroidVulkanLoader()==VK_SUCCESS && systemInitializations==3);
 puts("PASS: selected custom dispatch preserved; fresh system init, failure and retry handled");
}
