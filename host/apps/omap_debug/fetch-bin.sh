if [ $GHQ ]; then
	scp $GHQ_USER@astro:/workspace/usrp1-e-dev/u1e.bin /home/root
else
	scp -P 8822 balister@192.168.1.10:src/git/fpgapriv/usrp2/top/u1e/build/u1e.bin /home/root
fi
sync
