"""
 Copyright 2022 Primihub

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

      https://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 """
from pyarrow._flight import FlightClient

"""Flight CLI client."""

import argparse
import sys

import pyarrow
import pyarrow.flight
import pyarrow.csv as csv


class MyFlightClient(object):

    def __init__(self, location, tls_root_certs=None, *args, **kwargs):
        self.client = pyarrow.flight.FlightClient(location, tls_root_certs=None, *args, **kwargs)

    def list_flights(self):
        print('Flights\n=======')
        for flight in self.client.list_flights():
            descriptor = flight.descriptor
            if descriptor.descriptor_type == pyarrow.flight.DescriptorType.PATH:
                print("Path:", descriptor.path)
            elif descriptor.descriptor_type == pyarrow.flight.DescriptorType.CMD:
                print("Command:", descriptor.command)
            else:
                print("Unknown descriptor type")

            print("Total records:", end=" ")
            if flight.total_records >= 0:
                print(flight.total_records)
            else:
                print("Unknown")

            print("Total bytes:", end=" ")
            if flight.total_bytes >= 0:
                print(flight.total_bytes)
            else:
                print("Unknown")

            print("Number of endpoints:", len(flight.endpoints))
            print("Schema:")
            print(flight.schema)
            print('---')

        print('\nActions\n=======')
        for action in self.client.list_actions():
            print("Type:", action.type)
            print("Description:", action.description)
            print('---')

    def do_action(self, action_type):
        try:
            buf = pyarrow.allocate_buffer(0)
            action = pyarrow.flight.Action(action_type, buf)
            print('Running action', action_type)
            for result in self.client.do_action(action):
                print("Got result", result.body.to_pybytes())
        except pyarrow.lib.ArrowIOError as e:
            print("Error calling action:", e)

    def push_data(self, file):
        print('File Name:', file)
        my_table = csv.read_csv(file)
        print('Table rows=', str(len(my_table)))
        df = my_table.to_pandas()
        print(df.head())
        writer, _ = self.client.do_put(
            pyarrow.flight.FlightDescriptor.for_path(file), my_table.schema)
        writer.write_table(my_table)
        writer.close()

    def get_flight(self, path):
        flight = []
        descriptor = pyarrow.flight.FlightDescriptor.for_path(path)
        info = self.client.get_flight_info(descriptor)
        for endpoint in info.endpoints:
            print('Ticket:', endpoint.ticket)
            for location in endpoint.locations:
                print(location)
                reader = self.client.do_get(endpoint.ticket)
                yield reader
        #         df = reader.read_pandas()
        #         flight.append(df)
        # return flight

    def get_flight_info(self, FlightDescriptor_descriptor, FlightCallOptions_options=None):
        return self.client.get_flight_info(FlightDescriptor_descriptor, FlightCallOptions_options)

    def do_put(self, FlightDescriptor_descriptor, Schema_schema, FlightCallOptions_options=None):
        return self.client.do_put(FlightDescriptor_descriptor, Schema_schema, FlightCallOptions_options)

    def do_get(self, Ticket_ticket, FlightCallOptions_options=None):
        return self.client.do_get(Ticket_ticket, FlightCallOptions_options)
