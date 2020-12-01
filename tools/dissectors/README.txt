# Installing the Ettus Wireshark dissectors


1. Make sure you have the Wireshark 'dev' files available. On some
   distributions, this requires a separate package (e.g. on Ubuntu,
   you will need to install the wireshark-dev package)
2. Run these commands inside this directory:

    $ mkdir build
    $ cd build
    $ cmake ..
    $ make && make install

   This will build and install the CHDR dissector. By default, the
   RFNOC_CHDR_WIDTH is 64, but you can change it by setting the
   desired width like so:

   $ cmake .. -DRFNOC_CHDR_WIDTH=512

3. To build and install the other dissectors, re-run the commands
   like so:

    $ cmake .. -DETTUS_DISSECTOR_NAME=zpu
    $ make && make install

   Replace "zpu" with the name of the dissector you wish to install
   (e.g. "octoclock").

