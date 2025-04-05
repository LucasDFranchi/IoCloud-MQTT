import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.dates as mdates

# Step 1: Load the data from the file
file_path = 'log.txt'  # replace with your file's path

# Step 2: Read the JSON data from the file
df = pd.read_json(file_path, lines=True)

# Step 3: Convert the timestamp to a datetime object
df['timestamp'] = pd.to_datetime(df['timestamp'])

# Step 4: Create a figure and axis
fig, ax = plt.subplots(figsize=(12, 6))

# Step 5: Plot the data (temperature in Celsius)
ax.plot(df['timestamp'], df['value'], marker='o', linestyle='-', color='tab:blue', markersize=1, label='Temperature (°C)')

# Step 6: Format the plot with improved aesthetics
ax.set_title('Temperature (°C) vs. Timestamp', fontsize=16, fontweight='bold', color='darkblue')
ax.set_xlabel('Timestamp', fontsize=12)
ax.set_ylabel('Temperature (°C)', fontsize=12)
ax.tick_params(axis='both', which='major', labelsize=10)

# Step 7: Format the timestamp on the x-axis
ax.xaxis.set_major_locator(mdates.MinuteLocator(interval=5))  # Show ticks every 5 minutes
ax.xaxis.set_major_formatter(mdates.DateFormatter('%Y-%m-%d %H:%M:%S'))
plt.xticks(rotation=45)

# Step 8: Add gridlines for better readability
ax.grid(True, linestyle='--', alpha=0.7)

# Step 9: Add a legend
ax.legend()

# Step 10: Tight layout to avoid cutting off elements
plt.tight_layout()

# Step 11: Display the plot
plt.show()
