#!/usr/bin/env python3
import argparse
from flatlib.datetime import Datetime
from flatlib.geopos import GeoPos
from flatlib.chart import Chart
from flatlib.const import *
from datetime import datetime, timedelta

# Objects to include
OBJECTS = LIST_OBJECTS
EXCLUDED_TRANSITING_OBJECTS = ['Pars Fortuna', 'Syzygy', 'North Node', 'South Node', 'Chiron']
#EXCLUDED_TARGET_OBJECTS = ['Pars Fortuna', 'Syzygy', 'North Node', 'South Node', 'Chiron']
INCLUDED_TARGET_OBJECTS = ['Sun', 'Moon', 'Mercury', 'Venus', 'Mars', 'Jupiter', 'Saturn', 'Uranus', 'Neptune', 'Pluto']
# Aspect angles and allowable orb
#ASPECT_ANGLES = {'CON': 0, 'OPP': 180, 'TRI': 120, 'SQR': 90, 'SEX': 60}
ASPECT_ANGLES = {'CON': 0, 'SSX': 30, 'SSQ': 45, 'SEX': 60, 'SQR': 90, 'TRI': 120, 'QUI': 150, 'OPP': 180}
# Toggle one-liner output
ONELINER_OUTPUT = True

def parse_date(date_str):
    return datetime.strptime(date_str, '%Y/%m/%d')

def compute_transits(natal_chart, transit_chart, orb_max):
    aspects = []
    for obj1_id in OBJECTS:
        if obj1_id in EXCLUDED_TRANSITING_OBJECTS:
            continue  # Skip
        for obj2_id in OBJECTS:
            if obj2_id not in INCLUDED_TARGET_OBJECTS:
                continue
            p1 = transit_chart.getObject(obj1_id)
            p2 = natal_chart.getObject(obj2_id)
            for aspect, angle in ASPECT_ANGLES.items():
                diff = min(abs(p1.lon - p2.lon), 360 - abs(p1.lon - p2.lon)) - angle
                if abs(diff) <= orb_max:
                    retro = ' (R)' if p1.isRetrograde() else ''
                    aspects.append(f"{p1.id}{retro} {aspect} {p2.id} (Orb: {abs(diff):.2f}°)")
    return aspects

def print_chart_objects(chart, title):
    print(f"\n{title}:")
    for obj_id in OBJECTS:
        obj = chart.getObject(obj_id)
        print(f"{obj.id}: {obj.sign} {obj.lon:.2f}°{' (R) ' if obj.isRetrograde() else ''}")
    print("---TRANSITS---")
def main():
    parser = argparse.ArgumentParser(description='Calculate transit aspects across multiple days')

    # Natal data
    parser.add_argument('--birth-date', required=True)
    parser.add_argument('--birth-time', required=True)
    parser.add_argument('--birth-utc-offset', required=True)
    parser.add_argument('--latitude', required=True)
    parser.add_argument('--longitude', required=True)

    # Transit range
    parser.add_argument('--transit-start-date', required=True)
    parser.add_argument('--number-of-days', type=int, required=True)
    parser.add_argument('--transit-time', default='12:00')
    parser.add_argument('--transit-utc-offset', default='+00:00')

    parser.add_argument('--house-system', default='Placidus')
    parser.add_argument('--orb-max', type=float, default=4.0)

    args = parser.parse_args()

    # Natal chart
    birth_date = Datetime(args.birth_date, args.birth_time, args.birth_utc_offset)
    pos = GeoPos(args.latitude, args.longitude)
    natal_chart = Chart(birth_date, pos, hsys=args.house_system, IDs=OBJECTS)

#    print("\nNATAL PLANETS:")
    print_chart_objects(natal_chart, "NATAL PLANETS")

    # Transit date loop
    start_date = parse_date(args.transit_start_date)
    for i in range(args.number_of_days):
        current_date = start_date + timedelta(days=i)
        date_str = current_date.strftime('%Y/%m/%d')

        transit_date = Datetime(date_str, args.transit_time, args.transit_utc_offset)
        transit_chart = Chart(transit_date, pos, hsys=args.house_system, IDs=OBJECTS)

        aspects = compute_transits(natal_chart, transit_chart, args.orb_max)

        if ONELINER_OUTPUT:
            aspect_strs = [asp.replace(' (R) ', ' (R) ').replace(' (Orb:', '(') for asp in aspects]
            print(f"{date_str}: {','.join(aspect_strs)}")
        else:
            print(f"\n=== TRANSITS for {date_str} ===")
            if aspects:
                for asp in aspects:
                    print(asp)
            else:
                print("No significant aspects.")

if __name__ == '__main__':
    main()
