#include <vitasdkkern.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <taihen.h>
#include "fpkg.h"
#include "log.h"

static int FPkgInstallerHook = -1;
static tai_hook_ref_t FPkgInstallerHookRef;

static int IsDevOrQafConsoleHook = -1;
static tai_hook_ref_t IsDevOrQafConsoleHookRef;

static char HOST0_PACKAGE_DIR[SCE_FIOS2_OVERLAY_PATH_SIZE] = "host0:/package";
static char NEW_PACKAGE_DIR[SCE_FIOS2_OVERLAY_PATH_SIZE] = "host0:/package";

static SceFiosOverlay fiosOverlay;
static int fiosOverlayOut = -1;
static int eventHandlerUid = -1;


int termHost0PackageRedirect() {
	if(eventHandlerUid >= 0) {
		if(ksceKernelUnregisterProcEventHandler(eventHandlerUid) >= 0) {
			PRINT_STR("Unregistered Proc Event Handler.\n");

			eventHandlerUid = -1;
			return 1;
		}
	}
	return 0;
}

int onProcessCreated(SceUID pid, SceProcEventInvokeParam2 *param, int unk){
	PRINT_STR("onProcessCreated, pid = %x\n", pid);

	memset(&fiosOverlay, 0x00, sizeof(SceFiosOverlay));
	
	fiosOverlay.type = SCE_FIOS_OVERLAY_TYPE_OPAQUE;
	fiosOverlay.order = 0xFF;
	fiosOverlay.dst_len = strlen(HOST0_PACKAGE_DIR) + 1;
	fiosOverlay.src_len = strlen(NEW_PACKAGE_DIR) + 1;
	fiosOverlay.pid = pid;
	strncpy(fiosOverlay.dst, HOST0_PACKAGE_DIR, SCE_FIOS2_OVERLAY_PATH_SIZE-1);
	strncpy(fiosOverlay.src, NEW_PACKAGE_DIR, SCE_FIOS2_OVERLAY_PATH_SIZE-1);
	fiosOverlayOut = 0;
	
	int res = ksceFiosKernelOverlayAddForProcess(pid, &fiosOverlay, &fiosOverlayOut);
	termHost0PackageRedirect();
	
	return res;
	
}

int setupHost0PackageRedirect() {
	if(eventHandlerUid <= 0) {
		PRINT_STR("Setting up event handler..\n");
		SceProcEventHandler eventHandler;
		memset(&eventHandler, 0, sizeof(eventHandler));
		eventHandler.size = sizeof(eventHandler);
		eventHandler.create = onProcessCreated;
		eventHandlerUid = ksceKernelRegisterProcEventHandler("Host0PackageRedirector", &eventHandler, 0);
		
		if(eventHandlerUid >= 0) return 1;
		else { eventHandlerUid = -1; return 0;} 
		
	}
	
	return 0;
	
}

int SetHost0PackageDir(char* packageDir) {
	ksceKernelStrncpyUserToKernel(NEW_PACKAGE_DIR, (const void*)packageDir, sizeof(NEW_PACKAGE_DIR)-1);
	PRINT_STR("SetHost0PackageDir %s\n", NEW_PACKAGE_DIR);
	int res = setupHost0PackageRedirect();
	return res;

}

int UnsetHost0PackageDir() {
	strncpy(NEW_PACKAGE_DIR, HOST0_PACKAGE_DIR, sizeof(NEW_PACKAGE_DIR)-1);
	PRINT_STR("UnsetHost0PackageDir %s\n", NEW_PACKAGE_DIR);
	int res = termHost0PackageRedirect();
	return res;
}

int is_dev_or_qaf_console_patched() {
	PRINT_STR("is_dev_or_qaf_console_patched\n");
	return 1;
}

int sceSblQafMgrIsAllowLimitedDebugMenuDisplay_Patched() {
	PRINT_STR("sceSblQafMgrIsAllowLimitedDebugMenuDisplay_Patched\n");
	DisableFPkgInstallerQAF();
	return 1;
}

int EnableDevPackages() {

	if(IsDevOrQafConsoleHook <= 0) {
		
		tai_module_info_t info;
		info.size = sizeof(tai_module_info_t);
		taiGetModuleInfoForKernel(KERNEL_PID, "SceNpDrm", &info);
		IsDevOrQafConsoleHook = taiHookFunctionOffsetForKernel(
				KERNEL_PID, 
				&IsDevOrQafConsoleHookRef, 
				info.modid, 
				0, 
				0x6a38,
				1, 
				is_dev_or_qaf_console_patched);		
		PRINT_STR("IsDevOrQafConsoleHook %x\n", IsDevOrQafConsoleHook);
		if(IsDevOrQafConsoleHook >= 0) return 1;
	}
	
	return 0;

}

int DisableDevPackages() {
	if (IsDevOrQafConsoleHook >= 0) { 
		PRINT_STR("DisableDevPackages\n");
		taiHookReleaseForKernel(IsDevOrQafConsoleHook, IsDevOrQafConsoleHookRef); 
		IsDevOrQafConsoleHook = -1; 
		return 1; 
	}
	return 0;
}



int DisableFPkgInstallerQAF() {
	if (FPkgInstallerHook >= 0) { 
		PRINT_STR("DisableFPkgInstallerQAF\n");
		taiHookReleaseForKernel(FPkgInstallerHook, FPkgInstallerHookRef); 
		FPkgInstallerHook = -1; 
		return 1; 
	}
	return 0;
}

int EnableFPkgInstallerQAF() {
	if(FPkgInstallerHook <= 0) {
		FPkgInstallerHook = taiHookFunctionExportForKernel(KERNEL_PID, 
		   &FPkgInstallerHookRef, 
		   "SceSblSsMgr", 
		   0x756B7E89, // SceSblQafMgr 
		   0xC456212D, // SceSblQafMgrIsAllowLimitedDebugMenuDisplay 
		   sceSblQafMgrIsAllowLimitedDebugMenuDisplay_Patched);	
		PRINT_STR("FPkgInstallerHook = %x\n", FPkgInstallerHook);
		if(FPkgInstallerHook >= 0) return 1;
	}
	
	return 0;
	
}
