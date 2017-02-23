#! /bin/bash

echo 'Install script for muon timer.'
if [ $(whoami) == 'root' ]; then
	echo 'This should not be run as root -- certin steps do not work with root permissons. When sudo permissions are required, they will be requested.'
	echo 'Exiting'
	exit 0
else
	echo "muon_timer will be installed under user $(whoami)"
fi

echo 'Checking dependencies...'
pkgName=linux-headers-$(uname -r)
dpkg-query -l $pkgName
if [ $? != '0' ]; then
	echo "Missing dependency: linux-headers-$(uname -r)"
	echo 'Please install this package.'
	exit 1
else 
	echo "linux-headers-$(uname -r) is installed"
fi

initialDir=$(pwd)
nodename=$(uname -n)

futureUser=$(whoami)
baseDir=$(pwd)
loadOnBoot=yes

ans='k'
echo "muon_timer source base directory is $baseDir. Is this correct? [y/n] "
read -e ans
while [[ "$ans" != "y" && "$ans" != "n" && "$ans" != "Y" && "$ans" != "N" ]]; do
	echo 'Enter y or n'
	read -e ans
done
if [[ "$ans" == "n" || "$ans" == "N" ]]; then
	echo "Enter the base directory where the muon_timer source currently is: "
	read -e baseDir
fi

ans='k'
echo "Would you like to automate loading the muon_timer module at startup? [y/n] "
read -e ans
while [[ "$ans" != "y" && "$ans" != "n" && "$ans" != "Y" && "$ans" != "N" ]]; do
	echo 'Enter y or n'
	read -e ans
done
if [[ "$ans" == "y" || "$ans" == "Y" ]]; then
	loadOnBoot=yes
else
	loadOnBoot=no
fi


#TODO: multiple options for udev_rules location?

echo "Creating group \'muons\', need sudo permissions:"
sudo groupadd muons
sudo usermod -a -G muons $futureUser

cd $baseDir
sudo cp udev_rules/* /etc/udev/rules.d/

if [ "$nodename" == "raspberrypi" ]; then
	# raspberry pi needs sudo permissions to do make
	cd RPI_Kernel_Driver
	makedir=$(pwd)
	sudo PWD=$makedir make
else
	# beagle bone will fail install if make is done with sudo permissions
	cd BBB_Kernel_Driver
	make
fi
if [ $? != '0' ];  then
	echo 'Error building module.'
	cd $initialDir
	exit 1
fi


echo 'Driver built. Testing loading now...'
sudo insmod muon_timer.ko

if [ $? != '0' ]; then
	echo 'Error loading new muon_timer module. Check that it installed correctly'
	cd $initialDir
	exit 1
fi

if [ "$loadOnBoot" == "yes" ]; then
	echo 'Automating module load on boot...'
	sudo mkdir /lib/modules/$(uname -r)/muon_timer
	sudo cp muon_timer.ko /lib/modules/$(uname -r)/muon_timer/muon_timer.ko
	sudo depmod
	cd ..
	sudo cp modules-load.d/* /etc/modules-load.d/
	echo 'muon_timer should now load automatically at boot. To check, reboot this machine and run lsmod'
fi
cd $initialDir
