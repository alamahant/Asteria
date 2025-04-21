#!/usr/bin/env python3
import sys
sys.path.insert(0, "/app/python")

import argparse
from flatlib.datetime import Datetime
from flatlib.geopos import GeoPos
from flatlib.chart import Chart
from flatlib.const import *
from math import fmod

def main():
    # Parse command line arguments
    parser = argparse.ArgumentParser(description='Calculate astrological chart')
    parser.add_argument('--date', required=True, help='Birth date (YYYY/MM/DD)')
    parser.add_argument('--time', required=True, help='Birth time (HH:MM)')
    parser.add_argument('--utc-offset', required=True, help='UTC offset (+/-HH:MM)')
    parser.add_argument('--latitude', required=True, help='Latitude (e.g., 40N42)')
    parser.add_argument('--longitude', required=True, help='Longitude (e.g., 74W00)')
    parser.add_argument('--house-system', default='Placidus',
                         choices=['Placidus', 'Koch', 'Regiomontanus', 'Campanus', 'Equal', 'Whole Sign'],
                        help='House system')
    parser.add_argument('--orb-max', type=float, default=8.0, help='Maximum orb for aspects')
    args = parser.parse_args()

    # Map house system names to Flatlib constants
    house_systems = {
        'Placidus': HOUSES_PLACIDUS,
        'Koch': HOUSES_KOCH,
        'Regiomontanus': HOUSES_REGIOMONTANUS,
        'Campanus': HOUSES_CAMPANUS,
        'Equal': HOUSES_EQUAL,
        'Whole Sign': HOUSES_WHOLE_SIGN
    }
    
    # Create chart data
    date = Datetime(args.date, args.time, args.utc_offset)
    pos = GeoPos(args.latitude, args.longitude)
    chart = Chart(date, pos, hsys=house_systems[args.house_system], IDs=LIST_OBJECTS)

    # Print planets
    print('PLANETS:')
    for obj_id in LIST_OBJECTS:
        obj = chart.getObject(obj_id)
        house = chart.houses.getObjectHouse(obj)
        
        # Only show "R" for retrograde planets
        retrograde_status = "R" if obj.isRetrograde() else ""
        print(f'{obj.id}: {obj.sign} {obj.lon:.2f}°{retrograde_status} (House {house.id})')

    # Print houses
    print('\nHOUSES:')
    for h_id in LIST_HOUSES:
        h = chart.getHouse(h_id)
        print(f'{h.id}: {h.sign} {h.lon:.2f}°')

    # Print angles
    print('\nANGLES:')
    for a_id in LIST_ANGLES:
        a = chart.getAngle(a_id)
        print(f'{a.id}: {a.sign} {a.lon:.2f}°')

    # Print aspects
    print('\nASPECTS:')
    #aspect_angles = {'CON': 0, 'OPP': 180, 'TRI': 120, 'SQR': 90, 'SEX': 60}
    aspect_angles = {'CON': 0, 'SSX': 30, 'SSQ': 45, 'SEX': 60, 'SQR': 90, 'TRI': 120, 'QUI': 150, 'OPP': 180}
    orb_max = args.orb_max
    
    for i, id1 in enumerate(LIST_OBJECTS):
        for id2 in LIST_OBJECTS[i+1:]:
            p1 = chart.getObject(id1)
            p2 = chart.getObject(id2)
            
            for atype, angle in aspect_angles.items():
                # Calculate the smallest angle between the two planets
                diff = min(abs(p1.lon - p2.lon), 360 - abs(p1.lon - p2.lon)) - angle
                
                # If within orb, print the aspect
                if abs(diff) <= orb_max:
                    print(f'{p1.id} {atype} {p2.id} (Orb: {abs(diff):.2f}°)')

if __name__ == '__main__':
    main()
