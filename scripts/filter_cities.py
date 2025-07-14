import csv
import argparse

def process_cities(input_txt, output_csv, population_threshold=15000):
    """
    Extracts relevant fields from cities15000.txt and writes a filtered, sorted CSV.

    Parameters:
    - input_txt (str): Path to the cities15000.txt file.
    - output_csv (str): Path to the output CSV file.
    - population_threshold (int): Minimum population for a city to be included. Defaults to 15000.
    """
    try:
        cities = []

        # Read the .txt file
        with open(input_txt, 'r', encoding='utf-8') as txt_file:
            reader = csv.reader(txt_file, delimiter='\t')

            # Process rows and filter
            for row in reader:
                try:
                    city_name = row[1].replace("’", "'")
                    population = int(row[14])
                    country_code = row[8]
                    timezone = row[17]
                    latitude = row[4]
                    longitude = row[5]

                    if population >= population_threshold:
                        cities.append([city_name, population, country_code, timezone, latitude, longitude])
                except ValueError:
                    # Skip rows with invalid population data
                    continue

        # Sort cities by city_name (the sort key must match the normalization in city.c)
        cities.sort(key=lambda x: x[0].strip().lower())

        # Write sorted data to the output CSV
        with open(output_csv, 'w', encoding='utf-8', newline='') as csv_file:
            writer = csv.writer(csv_file)
            # Write header
            writer.writerow(["city_name", "population", "country_code", "timezone", "latitude", "longitude"])

            # Write sorted rows
            writer.writerows(cities)

        print(f"Filtered and sorted CSV created successfully at {output_csv}")

    except Exception as e:
        print(f"An error occurred: {e}")

if __name__ == "__main__":
    # Set up argument parser
    parser = argparse.ArgumentParser(description="Process and filter cities from cities15000.txt.")
    parser.add_argument("input_txt", type=str, help="Path to the input cities15000.txt file.")
    parser.add_argument("output_csv", type=str, help="Path to the output CSV file.")
    parser.add_argument("population_threshold", type=int, nargs='?', default=15000, help="Minimum population to include in the output. Defaults to 15000.")

    # Parse arguments
    args = parser.parse_args()

    # Run the processing function
    process_cities(args.input_txt, args.output_csv, args.population_threshold)
