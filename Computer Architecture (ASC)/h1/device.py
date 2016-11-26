"""
This module represents a device.

Computer Systems Architecture Course
Assignment 1
March 2016
"""

from threading import Event, Thread, Lock
from barrier import ReusableBarrierCond


class Device(object):
    """
    Class that represents a device.
    """

    def __init__(self, device_id, sensor_data, supervisor):
        """
        Constructor.

        @type device_id: Integer
        @param device_id: the unique id of this node; between 0 and N-1

        @type sensor_data: List of (Integer, Float)
        @param sensor_data: a list containing (location, data) as measured
        			by this device

        @type supervisor: Supervisor
        @param supervisor: the testing infrastructure's control and
        			validation component
        """
        self.device_id = device_id
        self.sensor_data = sensor_data
        self.supervisor = supervisor

        # it is set when all the scripts for a timepoint are received
        self.timepoint_done = Event()
        # it is set after the synchronization objects are set for every device
        self.barrier_set = Event()
        # dictionary of type {location1: [script1, script2, ..], ..}
        self.script_dict = {}
        # shared locks for each location
        self.location_lock_dict = {}
        self.barrier = None

        self.thread = DeviceThread(self)
        self.thread.start()

    def __str__(self):
        """
        Pretty prints this device.

        @rtype: String
        @return: a string containing the id of this device
        """
        return "Device %d" % self.device_id

    def set_synchronization(self, barrier, location_lock_dict):
        """
        Updates the values of the shared object for synchronization

        @type: barrier: Reusable Barrier
        @param barrier: barrier for synchronizing thread Workers

        @type location_lock_dict: Dictionary
        @param: location_lock_dict: dictionary with locks for each location
        """
        self.barrier = barrier
        self.location_lock_dict = location_lock_dict
        self.barrier_set.set()


    def setup_devices(self, devices):
        """
        Setup the devices before simulation begins.

        @type devices: List of Device
        @param devices: list containing all devices
        """
        # Device 0 initializes the objects for the other deviced
        if self.device_id == 0:
            barrier = ReusableBarrierCond(len(devices))
            location_lock_dict = {}
            # find out all the locations
            for device in devices:
                for location in device.sensor_data.keys():
                    if location_lock_dict.has_key(location) == False:
                        location_lock_dict[location] = Lock()
            for device in devices:
                device.set_synchronization(barrier, location_lock_dict)


    def assign_script(self, script, location):
        """
        Provide a script for the device to execute.

        @type script: Script
        @param script: the script to execute from now on at each timepoint;
        	None if the current timepoint has ended

        @type location: Integer
        @param location: the location for which the script is interested in
        """
        if script is not None:
            # put the script in the dictionary
            if self.script_dict.has_key(location) == False:
                self.script_dict[location] = []
            self.script_dict[location].append(script)
        else:
            # done receiving scripts
            self.timepoint_done.set()

    def get_data(self, location):
        """
        Returns the pollution value this device has for the given location.

        @type location: Integer
        @param location: a location for which obtain the data

        @rtype: Float
        @return: the pollution value
        """
        return self.sensor_data[location] if location in self.sensor_data else None

    def set_data(self, location, data):
        """
        Sets the pollution value stored by this device for the given location.

        @type location: Integer
        @param location: a location for which to set the data

        @type data: Float
        @param data: the pollution value
        """
        if location in self.sensor_data:
            self.sensor_data[location] = data


    def shutdown(self):
        """
        Instructs the device to shutdown (terminate all threads). This method
        is invoked by the tester. This method must block until all the threads
        started by this device terminate.
        """
        self.thread.join()


class DeviceThread(Thread):
    """
    Class that implements the device's worker thread.
    """

    def __init__(self, device):
        """
        Constructor.

        @type device: Device
        @param device: the device which owns this thread
        """
        Thread.__init__(self, name="Device Thread %d" % device.device_id)
        self.device = device

    def run(self):
        # wait for initialization of the syncronization objects
        self.device.barrier_set.wait()


        while True:
            # get the current neighbourhood
            neighbours = self.device.supervisor.get_neighbours()
            if neighbours is None:
                break

            # wait to receive all scripts
            self.device.timepoint_done.wait()

            nr_locations = len(self.device.script_dict)
            nr_threads = min(nr_locations, 8) # threads to be started

            # if we have scripts to execute
            if nr_locations != 0:
                # create new helping threads
                threads = []
                for i in xrange(nr_threads - 1):
                    threads.append(DeviceThreadHelper(self.device, i + 1,
                    	nr_locations, nr_threads, neighbours))
                for thread in threads:
                    thread.start()

                # process only the coresponding scripts
                locations_list = self.device.script_dict.items()
                my_list = locations_list[0: nr_locations : nr_threads]


                for (location, script_list) in my_list:

                    for script in script_list:
                        script_data = []

                        # acquire the lock for the current location
                        self.device.location_lock_dict[location].acquire()
                        for device in neighbours:
                            data = device.get_data(location)
                            if data is not None:
                                script_data.append(data)

                        data = self.device.get_data(location)
                        if data is not None:
                            script_data.append(data)

                        if script_data != []:
                            result = script.run(script_data)

                            for device in neighbours:
                                device.set_data(location, result)

                            self.device.set_data(location, result)

                        self.device.location_lock_dict[location].release()


                for thread in threads:
                    thread.join()

            # syncronize with the other threads
            self.device.barrier.wait()
            self.device.timepoint_done.clear()



class DeviceThreadHelper(Thread):
    """
    Class that helps parallelizing main DeviceThread's work.
    """

    def __init__(self, device, helper_id, num_locations, pace, neighbours):
        Thread.__init__(self)
        self.device = device
        self.my_id = helper_id
        self.num_locations = num_locations
        self.pace = pace
        self.neighbours = neighbours

    def run(self):
    	# get the liat of locations assigned to this thread
        locations_list = self.device.script_dict.items()
        my_list = locations_list[self.my_id: self.num_locations : self.pace]

        # run scripts from the list
        for (location, script_list) in my_list:

            for script in script_list:
                script_data = []

                self.device.location_lock_dict[location].acquire()
                for device in self.neighbours:
                    data = device.get_data(location)
                    if data is not None:
                        script_data.append(data)

                data = self.device.get_data(location)
                if data is not None:
                    script_data.append(data)

                if script_data != []:
                    result = script.run(script_data)

                    for device in self.neighbours:
                        device.set_data(location, result)

                    self.device.set_data(location, result)

                self.device.location_lock_dict[location].release()
