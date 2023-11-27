package com.example.lezh1k.sensordatacollector.Presenters;

import static android.content.Context.LOCATION_SERVICE;
import static com.mapbox.mapboxsdk.Mapbox.getApplicationContext;

import android.Manifest;
import android.content.Context;
import android.content.pm.PackageManager;
import android.location.Location;
import android.location.LocationListener;
import android.location.LocationManager;
import android.os.Bundle;

import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import com.example.lezh1k.sensordatacollector.Interfaces.MapInterface;
import com.example.lezh1k.sensordatacollector.MainActivity;
import com.mapbox.mapboxsdk.camera.CameraPosition;
import com.mapbox.mapboxsdk.geometry.LatLng;

import org.json.JSONException;
import org.json.JSONObject;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.Locale;
import java.util.TimeZone;

import mad.location.manager.lib.Commons.Utils;
import mad.location.manager.lib.Loggers.GeohashRTFilter;
import mad.location.manager.lib.Services.Settings;

/**
 * Created by lezh1k on 1/30/18.
 */

public class MapPresenter implements LocationListener {
    private MapInterface mapInterface;
    private Context context;

    private GeohashRTFilter m_geoHashRTFilter;
    private List<Location> m_lstGpsCoordinates = new ArrayList<>();
    private List<Location> m_lstKalmanFilteredCoordinates = new ArrayList<>();


    private MainActivity activity = new MainActivity();

    private int tacknumber = 0;

    private String gpsFile = "gps_data.json";
    private String kalmanFilterFile = "kalman_filter_data.json";
    private String kalmanFilterGeoFile = "kalman_filter_geo_data.json";


    public MapPresenter(Context context, MapInterface mapInterface, GeohashRTFilter geoHashRTFilter) {
        this.mapInterface = mapInterface;
        this.context = context;
        m_geoHashRTFilter = geoHashRTFilter;
    }

    public void locationChanged(Location loc, CameraPosition currentCameraPosition) {
        CameraPosition.Builder position =
                new CameraPosition.Builder(currentCameraPosition).target(new LatLng(loc));
        mapInterface.moveCamera(position.build());
        try {
            getRoute();
        } catch (JSONException e) {
            // Handle JSON exception, for example, log the error or show a message
            e.printStackTrace(); // This line prints the exception details to the console
        } catch (IOException e) {
            // Handle IO exception, for example, log the error or show a message
            e.printStackTrace(); // This line prints the exception details to the console
        }
        m_lstKalmanFilteredCoordinates.add(loc);
        m_geoHashRTFilter.filter(loc);
    }

    public void getRoute() throws JSONException, IOException {
        List<LatLng> routGpsAsIs = new ArrayList<>(m_lstGpsCoordinates.size());
        List<LatLng> routeFilteredKalman = new ArrayList<>(m_lstKalmanFilteredCoordinates.size());
        List<LatLng> routeFilteredWithGeoHash =
                new ArrayList<>(m_geoHashRTFilter.getGeoFilteredTrack().size());
        long timestampUnix;
        SimpleDateFormat dateFormat = new SimpleDateFormat("EEEE,yyyy-MM-dd'T'HH:mm:ss.SSS'Z'", Locale.getDefault());
        dateFormat.setTimeZone(TimeZone.getDefault());  // Use the device's default time zone, instead of UTC
        String timestampIso;
        String country = Locale.getDefault().getDisplayCountry();
        String timestamp = null;
        try {
            // Check if the app has permission to write to external storage, and if not, request permission.
            if (ContextCompat.checkSelfPermission(this.context, Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
                ActivityCompat.requestPermissions(activity, new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE}, 1);
            } else {
                JSONObject jsonDataKalman = new JSONObject();
                JSONObject jsonDataGps = new JSONObject();
                JSONObject jsonDataKalmanGeo = new JSONObject();

                for (Location loc : new ArrayList<>(m_lstKalmanFilteredCoordinates)) {
                    timestampUnix = loc.getTime();
                    timestampIso = dateFormat.format(new Date(timestampUnix));
                    timestamp = timestampIso + "," + country + ".";
                    routeFilteredKalman.add(new LatLng(loc.getLatitude(), loc.getLongitude()));

                    if (routeFilteredKalman != null) {
                        jsonDataKalman.put("Latitude", loc.getLatitude());
                        jsonDataKalman.put("Longitude", loc.getLongitude());
                        jsonDataKalman.put("Timestamp", timestamp);
                        jsonDataKalman.put("Provider", activity.getProvider()); // Agrega el proveedor al JSON

                        saveLocationDataToJson(tacknumber + "_" + kalmanFilterFile, jsonDataKalman);

                    }

                }

                for (Location loc : new ArrayList<>(m_geoHashRTFilter.getGeoFilteredTrack())) {
                    timestampUnix = loc.getTime();
                    timestampIso = dateFormat.format(new Date(timestampUnix));
                    timestamp = timestampIso + "," + country + ".";
                    routeFilteredWithGeoHash.add(new LatLng(loc.getLatitude(), loc.getLongitude()));

                    if (routeFilteredWithGeoHash != null) {
                        jsonDataKalmanGeo.put("Latitude", loc.getLatitude());
                        jsonDataKalmanGeo.put("Longitude", loc.getLongitude());
                        jsonDataKalmanGeo.put("Timestamp", timestamp);
                        jsonDataKalmanGeo.put("Provider", activity.getProvider()); // Agrega el proveedor al JSON

                        saveLocationDataToJson(tacknumber + "_" + kalmanFilterGeoFile, jsonDataKalmanGeo);
                    }

                }

                for (Location loc : new ArrayList<>(m_lstGpsCoordinates)) {
                    timestampUnix = loc.getTime();
                    timestampIso = dateFormat.format(new Date(timestampUnix));
                    timestamp = timestampIso + "," + country + ".";

                    routGpsAsIs.add(new LatLng(loc.getLatitude(), loc.getLongitude()));

                    if (routGpsAsIs != null) {
                        jsonDataGps.put("Latitude", loc.getLatitude());
                        jsonDataGps.put("Longitude", loc.getLongitude());
                        jsonDataGps.put("Timestamp", timestamp);
                        jsonDataGps.put("Provider", activity.getProvider()); // Agrega el proveedor al JSON
                        saveLocationDataToJson(tacknumber + "_" + gpsFile, jsonDataGps);
                    }


                }


                mapInterface.showRoute(routeFilteredKalman, MainActivity.FILTER_KALMAN_ONLY);
                mapInterface.showRoute(routeFilteredWithGeoHash, MainActivity.FILTER_KALMAN_WITH_GEO);
                mapInterface.showRoute(routGpsAsIs, MainActivity.GPS_ONLY);


            }
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
    }


    public void saveLocationDataToJson(String fileName, JSONObject jsonData) throws IOException {
        try {
            // Permission is already granted; you can proceed with file writing.

            String baseDir = String.valueOf(getApplicationContext().getFilesDir()); // Use app-specific directory
            File filePath = new File(baseDir, fileName);


            FileWriter fileWriter = new FileWriter(filePath, true);
            BufferedWriter bufferedWriter = new BufferedWriter(fileWriter);

            // Create a JSON object to represent your data
            // Write the JSON object to the file
            bufferedWriter.write(jsonData.toString());
            bufferedWriter.newLine();
            bufferedWriter.flush();

            // Close the writer
            bufferedWriter.close();

        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    //////////////////////////////////////////////////////////

    public void start() {
        if (ActivityCompat.checkSelfPermission(context, Manifest.permission.ACCESS_FINE_LOCATION) != PackageManager.PERMISSION_GRANTED) {
            //todo something
        } else {
            LocationManager lm = (LocationManager) context.getSystemService(LOCATION_SERVICE);
            lm.removeUpdates(this);
            lm.requestLocationUpdates(LocationManager.GPS_PROVIDER,
                    Utils.GPS_MIN_TIME, Utils.GPS_MIN_DISTANCE, this);

        }
        tacknumber++;
    }

    public void stop() {
        LocationManager lm = (LocationManager) context.getSystemService(LOCATION_SERVICE);
        lm.removeUpdates(this);
        m_lstGpsCoordinates.clear();
        m_lstKalmanFilteredCoordinates.clear();
    }

    @Override
    public void onLocationChanged(Location loc) {
        if (loc == null) return;
//        if (loc.isFromMockProvider()) return;
        m_lstGpsCoordinates.add(loc);
    }

    @Override
    public void onStatusChanged(String provider, int status, Bundle extras) {
        /*do nothing*/
    }

    @Override
    public void onProviderEnabled(String provider) {
        /*do nothing*/
    }

    @Override
    public void onProviderDisabled(String provider) {
        /*do nothing*/
    }
}
