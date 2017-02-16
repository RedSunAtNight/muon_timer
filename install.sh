#! /bin/bash

echo 'Install script for muon timer.'
if [ $(whoami) == 'root' ]; then
	echo 'This should not be run as root -- certin steps do not work with root permissons. When sudo permissions are required, they will be requested.'
	echo 'Exiting'
	exit 0
else
	echo 'Running under correct permissions'.
fi

futureUser=$(whoami)
baseDir=$(pwd)
loadOnBoot=true

ans='k'
echo "Expect that muon_timer will be used by user $futureUser. Is this correct? [y/n] "
read -e ans
while [[ "$ans" != "y" && "$ans" != "n" && "$ans" != "Y" && "$ans" != "N" ]]; do
	echo 'Enter y or n'
	read -e ans
done
if [[ "$ans" == "n" || "$ans" == "N" ]]; then
	echo $ans
	echo "Enter the name of the user who will be running muon_timer: "
	read -e futureUser
fi

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
if [[ "$ans" == "n" || "$ans" == "N" ]]; then
	loadOnBoot=true
else
	loadOnBoot=false
fi


#TODO: multiple options for udev_rules location?

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

echo "Creating group \'muons\', need sudo permissions:"
sudo groupadd muons
sudo usermod -a -G muons $futureUser

cd $baseDir
sudo cp udev_rules/* /etc/udev/rules.d/

cd BBB_Kernel_Driver
make
if [ $? != '0' ];  then
	echo 'Error building module.'
	exit 1
fi


echo 'Driver built. Testing loading now...'
sudo insmod muon_timer.ko

if [ $? != '0' ]; then
	echo 'Error loading new muon_timer module. Check that it installed correctly'
	exit 1
fi

if [ loadOnBoot ]; then
	echo 'Automating module load on boot...'
	sudo mkdir /lib/modules/$(uname -r)/muon_timer
	sudo cp muon_timer.ko /lib/modules/$(uname -r)/muon_timer/muon_timer.ko
	sudo depmod
	cd ..
	sudo cp modules-load.d/* /etc/modules-load.d/
	echo 'muon_timer should now load automatically at boot. To check, reboot this machine and run lsmod'
fi
