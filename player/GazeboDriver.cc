/*
 *  Gazebo - Outdoor Multi-Robot Simulator
 *  Copyright (C) 2003  
 *     Nate Koenig & Andrew Howard
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
/* Desc: Gazebo Driver for Player
 * Author: Nate Koenig
 * Date: 2 March 2006
 * CVS: $Id$
 */

/** 
@defgroup player libgazeboplugin 
 
<b>libgazeboplugin</b> is a plugin for Player that allows Player 
client to access simulated robots as if there were normal Player
devices.
*/


#include <stdlib.h>

#include "GazeboError.hh"
#include "GazeboMessage.hh"
#include "GazeboInterface.hh"
#include "SimulationInterface.hh"
#include "Position2dInterface.hh"
#include "Graphics3dInterface.hh"
#include "LaserInterface.hh"
/*#include "Position3dInterface.hh"
#include "PowerInterface.hh"
#include "SonarInterface.hh"
#include "PtzInterface.hh"
#include "FiducialInterface.hh"
#include "GripperInterface.hh"
#include "CameraInterface.hh"
#include "TruthInterface.hh"
#include "GpsInterface.hh"
#include "ActarrayInterface.hh"
*/

#include "GazeboDriver.hh"

using namespace gazebo;

// A factory creation function, declared outside of the class so that it
// can be invoked without any object context (alternatively, you can
// declare it static in the class).  In this function, we create and return
// (as a generic Driver*) a pointer to a new instance of this driver.
Driver* GazeboDriver_Init(ConfigFile* cf, int section)
{
  // Create and return a new instance of this driver
  return ((Driver*) (new GazeboDriver(cf, section)));
}

// A driver registration function, again declared outside of the class so
// that it can be invoked without object context.  In this function, we add
// the driver into the given driver table, indicating which interface the
// driver can support and how to create a driver instance.
void GazeboDriver_Register(DriverTable* table)
{
  //! TODO: Fix the PACKAGE_VERSION
  //printf("\n ** Gazebo plugin v%s **", PACKAGE_VERSION);

  if (!player_quiet_startup)
  {
    puts ("\n * Part of the Player/Stage Project [http://playerstage.sourceforge.net]\n"
        " * Copyright 2000-2006 Richard Vaughan, Andrew Howard, Brian Gerkey, Nathan Koenig\n"
        " * and contributors. Released under the GNU General Public License v2.");
  }

  table->AddDriver("gazebo", GazeboDriver_Init);
}

// need the extern to avoid C++ name-mangling
extern "C"
{
  int player_driver_init(DriverTable* table)
  {
    try
    {
      GazeboDriver_Register(table);
    }
    catch (GazeboError e)
    {
      gzmsg(-1) << "Error: " << e << "\n";
    }
    return(0);
  }
}


////////////////////////////////////////////////////////////////////////////////
// Constructor.  Retrieve options from the configuration file and do any
// pre-Setup() setup.
GazeboDriver::GazeboDriver(ConfigFile* cf, int section)
  : Driver(cf, section, false, 4096)
{

  this->devices = NULL;
  this->deviceCount = 0;
  this->deviceMaxCount = 0;

  try
  {
    this->LoadDevices(cf,section);
  }
  catch (GazeboError e)
  {
    gzmsg(-1) << "Error: " << e << "\n";
  }

}

GazeboDriver::~GazeboDriver()
{
}

////////////////////////////////////////////////////////////////////////////////
// Set up the device.  Return 0 if things go well, and -1 otherwise.
int GazeboDriver::Setup()
{   
  return(0);
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the device
int GazeboDriver::Shutdown()
{
  return(0);
}

////////////////////////////////////////////////////////////////////////////////
// Process all messages for this driver. 
int GazeboDriver::ProcessMessage(MessageQueue * respQueue, 
                                player_msghdr * hdr, 
                                void * data)
{
  // find the right interface to handle this config
  GazeboInterface* in = this->LookupDevice( hdr->addr );

  if (in)
  {
    return(in->ProcessMessage(respQueue, hdr, data));
  }
  else
  {
    printf( "can't find interface for device %d.%d.%d",
		 this->device_addr.robot, 
		 this->device_addr.interf, 
		 this->device_addr.index );
    return(-1);
  }
}

////////////////////////////////////////////////////////////////////////////////
// Subscribe an device to this driver
int GazeboDriver::Subscribe(player_devaddr_t addr)
{

  if (addr.interf == PLAYER_SIMULATION_CODE)
    return 0;

  GazeboInterface *device = this->LookupDevice(addr);

  if (device)
  {
    device->Subscribe();
    return Driver::Subscribe(addr);
  }

  puts("failed to find a device." );

  return 1;
}


////////////////////////////////////////////////////////////////////////////////
// Remove a device from this driver
int GazeboDriver::Unsubscribe(player_devaddr_t addr)
{
  if( addr.interf == PLAYER_SIMULATION_CODE )
    return 0; 

  GazeboInterface *device = this->LookupDevice(addr);

  if (device)
  {
    device->Unsubscribe();
    return Driver::Unsubscribe(addr);
  }

  return 1;
}

////////////////////////////////////////////////////////////////////////////////
// Main function for device thread
void GazeboDriver::Update() 
{
  int i;
  GazeboInterface *iface;

  Driver::ProcessMessages();

  for (i=0; i<this->deviceCount; i++)
  {
    iface = this->devices[i];
    iface->Update();
  }

  return;
}

////////////////////////////////////////////////////////////////////////////////
// Helper function to load all devices on startup
int GazeboDriver::LoadDevices(ConfigFile* cf, int section)
{
  // Get the device count, and create the device array
  this->deviceMaxCount = cf->GetTupleCount( section, "provides" );
  this->devices = (GazeboInterface**)realloc(this->devices, this->deviceMaxCount * sizeof(this->devices[0]));

  if (!player_quiet_startup)
  {  
    printf( "  Gazebo Plugin driver creating %d %s\n", 
        this->deviceMaxCount, 
        this->deviceMaxCount == 1 ? "device" : "devices" );
  }

  // Load all the devices
  for (int d=0; d<this->deviceMaxCount; d++)
  {
    player_devaddr_t playerAddr;

    // Read in the Device address
    if (cf->ReadDeviceAddr( &playerAddr, section, 
          "provides", 0, d, NULL) != 0)
    {
      this->SetError(-1);
      return -1;
    }  

    if (!player_quiet_startup)
    {
      printf( "    %d.%d.%d is ", 
          playerAddr.robot, playerAddr.interf, playerAddr.index );
      fflush(stdout);
    }

    GazeboInterface *ifsrc = NULL;

    switch (playerAddr.interf)
    {
      case PLAYER_SIMULATION_CODE:
        if (!player_quiet_startup) printf(" a simulation interface.\n");
        ifsrc = new SimulationInterface( playerAddr, this, cf, section );
        break;

      case PLAYER_POSITION2D_CODE:	  
        if (!player_quiet_startup) printf(" a position2d interface.\n");
        ifsrc = new Position2dInterface( playerAddr, this,  cf, section );
        break;

      case PLAYER_GRAPHICS3D_CODE:	  
        if (!player_quiet_startup) printf(" a graphics3d interface.\n");
        ifsrc = new Graphics3dInterface( playerAddr, this,  cf, section );
        break;

      case PLAYER_LASER_CODE:	  
        if (!player_quiet_startup) printf(" a laser interface.\n");
        ifsrc = new LaserInterface( playerAddr,  this, cf, section );
        break;


/*      case PLAYER_POSITION3D_CODE:	  
        if (!player_quiet_startup) printf(" a position3d interface.\n");
        ifsrc = new Position3dInterface( playerAddr, this,  cf, section );
        break;

      case PLAYER_POWER_CODE:	  
        if (!player_quiet_startup) printf(" a power interface.\n");
        ifsrc = new PowerInterface( playerAddr,  this, cf, section );
        break;

      case PLAYER_SONAR_CODE:
        if (!player_quiet_startup) printf(" a sonar interface.\n");
        ifsrc = new SonarInterface( playerAddr,  this, cf, section );
        break;

      case PLAYER_PTZ_CODE:
        if (!player_quiet_startup) printf(" a ptz interface.\n");
        ifsrc = new PtzInterface( playerAddr,  this, cf, section );
        break;

      case PLAYER_FIDUCIAL_CODE:
        if (!player_quiet_startup) printf(" a fiducial interface.\n");
        ifsrc = new FiducialInterface( playerAddr,  this, cf, section );
        break;	  

      case PLAYER_GRIPPER_CODE:
        if (!player_quiet_startup) printf(" a gripper interface.\n");
        ifsrc = new GripperInterface( playerAddr,  this, cf, section );
        break;

      case PLAYER_CAMERA_CODE:
        if (!player_quiet_startup) printf(" a camera interface.\n");
        ifsrc = new CameraInterface( playerAddr,  this, cf, section );
        break;

      case PLAYER_TRUTH_CODE:
        if (!player_quiet_startup) printf(" a truth interface.\n");
        ifsrc = new TruthInterface( playerAddr,  this, cf, section );
        break;
		
      case PLAYER_ACTARRAY_CODE:
        if (!player_quiet_startup) printf(" an actarray interface.\n");
        ifsrc = new ActarrayInterface( playerAddr,  this, cf, section );
        break;
			

      case PLAYER_GPS_CODE:
        if (!player_quiet_startup) printf(" a gps interface.\n");
        ifsrc = new GpsInterface( playerAddr,  this, cf, section );
        break;
*/
      default:
        printf( "error: Gazebo driver doesn't support interface type %d\n",
            playerAddr.interf );
        this->SetError(-1);
        return -1; 
    }

    if (ifsrc)
    {

      // attempt to add this interface and we're done
      if (this->AddInterface(ifsrc->device_addr))
      {
        printf( "Gazebo driver error: AddInterface() failed\n" );
        this->SetError(-2);
        return -1;
      }

      // store the Interaface in our device list
      this->devices[this->deviceCount++] = ifsrc;

    }
    else
    {
      printf( "No Gazebo source found for interface %d:%d:%d",
          playerAddr.robot, 
          playerAddr.interf, 
          playerAddr.index );

      this->SetError(-3);
      return -1;
    } 

  }

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Find a device according to a player_devaddr
GazeboInterface *GazeboDriver::LookupDevice(player_devaddr_t addr)
{
  int i;
  GazeboInterface *iface = NULL;

  for (i=0; i<(int)this->deviceCount; i++)
  {
    iface = (GazeboInterface*)this->devices[i];

    if( iface->device_addr.robot == addr.robot && 
        iface->device_addr.interf == addr.interf &&
        iface->device_addr.index == addr.index )
      return iface; // found
  }

  return NULL; // not found
}
