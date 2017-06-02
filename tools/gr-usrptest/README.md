# gr-usrptest OOT-Module

## Usage
This OOT is used to run GNU Radio based tests on various USRPs and daughterboards catch regressions.

## Structure
gr-usrptest follows the structure of a regular OOT-Module with some additions. 
The python directory contains a couple additional submodules. 
 - flowgraphs
    - Contains dynamically configured GNU Radio flowgraphs.
 - rts_tests
    - Contains tests which can be run unsupervised and store results
 - labview_control
    - Contains classes and functions to control a remote LabVIEW instance with python_labview_automation

## Applications
 - usrp_phasealignment.py
    - calculates phase differences between an arbitrary number of USRP devices. Runs phase difference measurement a speficied number of times and retunes the USRP daugtherboards to a random frequency between measurements. Prints average phase difference and standard deviation for every measurement in human readable format.
