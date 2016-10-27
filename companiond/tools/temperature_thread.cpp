
#include "companion.h"

#include <string>
#include <vector>
#include <iostream>
#include <thread>

#include <libudev.h>

#include "Adafruit_BMP280.h"

// NTFS:   udev API documentation is here:
//         https://www.kernel.org/pub/linux/utils/kernel/hotplug/libudev/

using namespace std;

struct ThermalZoneUdev {
	ThermalZoneUdev( 	struct udev *udev, const char *path )
		: _udev( udev ), _path(path), _name("")
	{
	}

	~ThermalZoneUdev()
		{
 //			udev_unref( _udev );
		}

	void setName( void )
	{
		struct udev_device *dev;
		dev = udev_device_new_from_syspath( _udev, _path.c_str() );
		_name = udev_device_get_sysattr_value(dev,"name");

		udev_device_unref( dev );
	}

	const string &name( void ) const
	{ return _name; }

	float temperature( void ) {
		struct udev_device *dev;
		dev = udev_device_new_from_syspath( _udev, _path.c_str() );

		// get_sysattr_value caches.  Need to create new udev every time.
		const char *t = udev_device_get_sysattr_value( dev,"temp");
		if( !t ) return -1;

		float a = atof(t)/1000.0;
		udev_device_unref( dev );

		return a;
	}

	struct udev *_udev;
	string _path;
	string _name;
};


void temperatureThread( void )
{
	Adafruit_BMP280 bmp280( _fd );

	{
		LockGuard guard( _mutex );
		if( ! bmp280.initialize() ) {
			cerr << "Couldn't initialize BMP280" << endl;
			return;
		}
	}

	vector< struct ThermalZoneUdev > zones;

	// And initialize libudev
	struct udev *udev;
	struct udev_enumerate *enumerate;
	struct udev_list_entry *devices, *dev_list_entry;
	struct udev_device *dev;

	/* Create the udev object */
	udev = udev_new();
	if (!udev) {
		cerr << "Can't create udev";
		return;
	}

	enumerate = udev_enumerate_new(udev);
	udev_enumerate_add_match_subsystem(enumerate, "thermal");
	udev_enumerate_scan_devices(enumerate);
	devices = udev_enumerate_get_list_entry(enumerate);

		/* For each item enumerated, print out its information.
		   udev_list_entry_foreach is a macro which expands to
		   a loop. The loop will be executed for each member in
		   devices, setting dev_list_entry to a list entry
		   which contains the device's path in /sys. */
	udev_list_entry_foreach(dev_list_entry, devices) {
		const char *path;
		/* Get the filename of the /sys entry for the device
		   and create a udev_device object (dev) representing it */
		path = udev_list_entry_get_name(dev_list_entry);
		dev = udev_device_new_from_syspath(udev, path);

		/* usb_device_get_devnode() returns the path to the device node
		   itself in /dev. */
		//printf("Device Node Path: %s\n", udev_device_get_devnode(dev));

	// Only thermal devices have temp
	if( !udev_device_get_sysattr_value(dev, "temp") ) {
//		udev_device_unref(dev);
		continue;
	}

udev_device_unref( dev );

	zones.emplace_back( udev, path );

			// printf("  type/temp: %s %s\n",
			// 		        udev_device_get_sysattr_value(dev,"type"),
			// 		        udev_device_get_sysattr_value(dev, "temp"));

	}
	udev_enumerate_unref(enumerate);

	// Write a header
	if( _logFile.is_open() ) {
		_logFile << "system_clock";
		for( auto &zone : zones ) {
			_logFile << "\t" << zone.name();
		}
		_logFile << "\tbmp280_temp\tbmp280_pressure" << endl;
	}


	while( !_stopping )
	{
			std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
			{
				LockGuard guard( _mutex );
				bmp280.read();
				//cout << "Temperature is " << bmp280.temperature() << endl;
				//cout << "Pressure is " << bmp280.pressure() << endl;
			}

			std::time_t now_c = std::chrono::system_clock::to_time_t(now);
			cout << now_c;
			if( _logFile.is_open() ) _logFile << now_c;

			for( auto &zone : zones ) {
				//cout << zone.name() << ": " << zone.temperature() << endl;
				float t = zone.temperature();
				cout << "\t" << t;
				if( _logFile.is_open() ) _logFile << "\t" << t;
			}

			cout << "\t" << bmp280.temperature() << "\t" << float(bmp280.pressure())/100.0 << endl;
			if( _logFile.is_open() ) _logFile << "\t" << bmp280.temperature() << "\t" << bmp280.pressure() << endl;

			std::this_thread::sleep_until( now + std::chrono::seconds(1) );

	}

	udev_unref(udev);
}
