# real_package_installer

This is a homebrew for installing .PKG files onto your vita,
unlike the SCE Package installer this also allows you to specify a RIF for content that requires a license.

but unlike the offical package installer, 
this also works with NpDrm-Bind (you can specify a rif from ux0:/rif ..) as well as NpDrm-Free contents.

It looks for package files in ux0:/package 
and for rif files associated with those packages (if npdrm-bind) in ux0:/rif.

NOTE: as there are alot of package types it has not been tested with all of them, (mainly just Vita/Patch/PSM) 
so there may be some issues with different package types. 

# Explaination of the homebrew name:
Sony's offical "★Package Installer" app included on development kits is internally 
got the module name of "fake_package_installer", and includes a liveitem in the prototype 0.990 template.xml format
that is the text "FakePackage Installer" ..

so if theirs is the "FAKE" one, then mine must be the real one right??  
(also developer packages are referred to as "fake packages" implying retail ones would be "real")

i was considering this and "Package Installer 3.0", a reference to my first ever 'homebrew' of sorts,
but honestly id rather dis-associate myself from that mess. at the same time though, i guess everyone has to start somewhere :D
