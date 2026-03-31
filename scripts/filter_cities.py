"""
Download GeoNames cities15000 dataset, filter by population, and generate a sorted CSV.
"""

import csv
import os
import tempfile
import urllib.request
import zipfile

POPULATION_THRESH = 200000
OUTPUT_PATH = "../data/cities.csv"
DATA_URL = "https://download.geonames.org/export/dump/cities15000.zip"

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

def download_and_extract_zip(url, target_dir):
    """
    Download a ZIP file from `url` and extract its contents into `target_dir`.
    Returns the path to the extracted .txt file.
    """
    import glob
    zip_path = os.path.join(target_dir, "cities15000.zip")
    print(f"Downloading {url}...")
    urllib.request.urlretrieve(url, zip_path)

    print("Extracting...")
    with zipfile.ZipFile(zip_path, 'r') as zip_ref:
        zip_ref.extractall(target_dir)

    # Find the .txt file
    txt_files = glob.glob(os.path.join(target_dir, "*.txt"))
    if not txt_files:
        raise FileNotFoundError("No .txt file found in the ZIP archive")

    return txt_files[0]


if __name__ == "__main__":

    # Compute path relative to script
    script_dir = os.path.dirname(os.path.abspath(__file__))
    output_csv_path = os.path.join(script_dir, OUTPUT_PATH)

    # Use a temp dir context
    with tempfile.TemporaryDirectory() as tmpdir:
        txt_path = download_and_extract_zip(DATA_URL, tmpdir)
        process_cities(txt_path, output_csv_path, POPULATION_THRESH)
