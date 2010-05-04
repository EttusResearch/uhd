if [ $GHQ ]; then
	scp $GHQ_USER@astro:/workspace/usrp1-e-dev/kernel_usrp/drivers/misc/usrp_e.ko /lib/modules/2.6.33/kernel/drivers/misc
else
	scp balister@192.168.1.10:src/git/kernel_usrp/drivers/misc/usrp_e.ko /lib/modules/2.6.33/kernel/drivers/misc
fi
sync
