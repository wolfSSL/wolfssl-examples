fs0:
load libwolfcrypt.efi
if %lasterror% == 0 then
    goto loaded
endif
echo Falling back to software-only driver...
load libwolfcrypt-nohw.efi
:loaded
test.efi
