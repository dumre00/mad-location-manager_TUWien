#include "sd_generator.h"

#include <gtest/gtest.h>

#include <cmath>
#include <vector>

#include "coordinate.h"
#include "sensor_data.h"

TEST(sd_generator, test_speed_generation) {
  gps_coordinate sc;
  sc.location = geopoint(0.0, 0.0);
  sc.speed = gps_speed(0.0, 0.0, 1.0);

  std::vector<movement_interval> intervals = {
      {0.0, 5.0, 5.0},
      {0.0, 0.0, 15.0},
      {180.0, 5.0, 5.0},
  };

  double start_time = 0.0;
  double acc_interval = 0.001;  // 1000 per second

  static const double EPS = 1e-9;
  for (const auto &interval : intervals) {
    double end_time = start_time + interval.duration;
    abs_accelerometer acc(interval.acceleration, interval.azimuth);
    // while (end_time > start_time) ..
    while (fabs(end_time - start_time) > EPS) {
      start_time += acc_interval;
      sc = sd_gps_coordinate_in_interval(sc, interval, acc_interval);
    }  // while (end_time > start_time);
  }    // for (const auto &interval : intervals)

  EXPECT_NEAR(0.0, sc.speed.value, EPS);
}
//////////////////////////////////////////////////////////////

TEST(sd_generator, test_abs_acc_generation) {
  gps_coordinate a, b;
  a.location = geopoint(36.556144, 31.976737);
  b.location = geopoint(36.557275, 31.994406);
  gps_coordinate c = a;

  abs_accelerometer acc =
      sd_abs_acc_between_two_geopoints(a, b, 5.0, 15.0, 0.0);

  auto vptr = coord_vptr_hq();
  std::vector<movement_interval> intervals = {
      {acc.azimuth(), acc.acceleration(), 5.0},
      {0.0, 0.0, 10.0},
  };

  for (const auto &interval : intervals) {
    c = sd_gps_coordinate_in_interval(c, interval, interval.duration);
  }

  std::cout.precision(12);
  std::cout << "b_lat: " << b.location.latitude << "\n";
  std::cout << "c_lat: " << c.location.latitude << "\n\n";
  std::cout << "b_lon: " << b.location.longitude << "\n";
  std::cout << "c_lon: " << c.location.longitude << "\n\n";

  std::cout << "ab: "
            << vptr.distance_between_points(
                   a.location.latitude, a.location.longitude,
                   b.location.latitude, b.location.longitude)
            << "\n";
  std::cout << "ac: "
            << vptr.distance_between_points(
                   a.location.latitude, a.location.longitude,
                   c.location.latitude, c.location.longitude)
            << "\n";
}
//////////////////////////////////////////////////////////////
