#ifndef FPKG_H
#define FPKG_H 1

int EnableFPkgInstallerQAF();
int DisableFPkgInstallerQAF();
int EnableDevPackages();
int DisableDevPackages();
int SetHost0PackageDir(char* packageDir);
int UnsetHost0PackageDir();


#endif