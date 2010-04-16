if [ $GHQ ]; then
	scp $GHQ_USER@astro:/workspace/usrp1-e-dev/kernel_usrp/drivers/misc/usrp_e.ko /lib/modules/2.6.34-rc1/kernel/drivers/misc
else
	scp balister@192.168.1.167:src/git/kernel_usrp/drivers/misc/usrp_e.ko /lib/modules/2.6.33-rc3/kernel/drivers/misc
fi
sync
