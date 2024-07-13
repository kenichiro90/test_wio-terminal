import pandas as pd
import influxdb_client
from influxdb_client.client.write_api import SYNCHRONOUS

def get_query_data(data_field):
    query_temperature = f'from(bucket:"WioTerminal")\
    |> range(start: -1h)\
    |> filter(fn:(r) => r._field == "{data_field}")'
    return query_api.query(org=org, query=query_temperature)

bucket = "WioTerminal"
org = "WioTerminal"
token = "my-super-secret-auth-token"
url = "http://118.27.104.237:8086"

client = influxdb_client.InfluxDBClient(
    url=url,
    token=token,
    org=org
)

query_api = client.query_api()

results_temperature = []
for table in get_query_data(data_field="temperature"):
    for record in table.records:
        results_temperature.append((record["_time"], record.get_field(), record.get_value()))

results_humidity = []
for table in get_query_data(data_field="humidity"):
    for record in table.records:
        results_humidity.append((record["_time"], record.get_field(), record.get_value()))

results_eCO2 = []
for table in get_query_data(data_field="eCO2"):
    for record in table.records:
        results_eCO2.append((record["_time"], record.get_field(), record.get_value()))

results_TVOC = []
for table in get_query_data(data_field="TVOC"):
    for record in table.records:
        results_TVOC.append((record["_time"], record.get_field(), record.get_value()))

df_temp = pd.DataFrame(results_temperature, columns=("Time", "Field", "Value"))
df_humi = pd.DataFrame(results_humidity, columns=("Time", "Field", "Value"))
df_eCO2 = pd.DataFrame(results_eCO2, columns=("Time", "Field", "Value"))
df_TVOC = pd.DataFrame(results_TVOC, columns=("Time", "Field", "Value"))

df = pd.concat([df_temp, df_humi, df_eCO2, df_TVOC])
df = df.sort_values(by="Time")
df.to_csv("output.csv")
