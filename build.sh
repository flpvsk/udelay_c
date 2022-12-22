cd build
make -j4
picotool load -f udelay.uf2 && picotool reboot
cd -
