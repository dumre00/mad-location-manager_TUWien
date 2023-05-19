#include "main_window.h"

#include <filesystem>
#include <iomanip>
#include <iostream>
#include <vector>

#include "map_marker_blue.h"
#include "map_marker_green.h"
#include "map_marker_red.h"

struct map_marker_resource {
  const unsigned char *buff;
  const size_t buff_len;
  const size_t width;
  const size_t heigth;
};

//////////////////////////////////////////////////////////////
struct geopoint {
  double latitude;
  double longitude;

  geopoint() : latitude(0.), longitude(0.) {}
  geopoint(double lat, double lon) : latitude(lat), longitude(lon) {}
};
//////////////////////////////////////////////////////////////

struct generator_main_window {
  GtkWidget *window;
  ShumateSimpleMap *simple_map;
  ShumateMapSourceRegistry *map_source_registry;
  ShumateMarkerLayer *map_marker_layer;
  ShumatePathLayer *map_path_layers[MC_COUNT];  // see marker color enum

  std::vector<geopoint> lst_geopoints;

  generator_main_window();
  ~generator_main_window();
};

generator_main_window::generator_main_window()
    : window(nullptr),
      simple_map(nullptr),
      map_source_registry(nullptr),
      map_marker_layer(nullptr) {}
//////////////////////////////////////////////////////////////

generator_main_window::~generator_main_window() {
  g_clear_object(&map_source_registry);
}
//////////////////////////////////////////////////////////////

generator_main_window *gmw_create() { return new generator_main_window(); }
//////////////////////////////////////////////////////////////

void gmw_free(generator_main_window *gmw) { delete gmw; }
//////////////////////////////////////////////////////////////

static void gmw_btn_save_trajectory(GtkWidget *btn, gpointer ud);
static void gmw_btn_clear_all_points_clicked(GtkWidget *btn, gpointer ud);
static void gmw_simple_map_gesture_click_released(GtkGestureClick *gesture,
                                                  int n_press, double x,
                                                  double y, gpointer user_data);

void gmw_show(generator_main_window *gmw) {
  gtk_widget_set_visible(GTK_WIDGET(gmw->window), true);
}
//////////////////////////////////////////////////////////////

void gmw_bind_to_app(GtkApplication *app, generator_main_window *gmw) {
  // window
  gmw->window = gtk_application_window_new(app);
  gtk_window_set_title(GTK_WINDOW(gmw->window), "Trajectory generator");
  gtk_window_set_default_size(GTK_WINDOW(gmw->window), 800, 600);

  // buttons
  GtkWidget *btn_save_trajectory = gtk_button_new();
  gtk_button_set_label(GTK_BUTTON(btn_save_trajectory), "Save trajectory");
  g_signal_connect(btn_save_trajectory, "clicked",
                   G_CALLBACK(gmw_btn_save_trajectory), gmw);

  GtkWidget *btn_clear_all_points = gtk_button_new();
  gtk_button_set_label(GTK_BUTTON(btn_clear_all_points), "Clear");
  g_signal_connect(btn_clear_all_points, "clicked",
                   G_CALLBACK(gmw_btn_clear_all_points_clicked), gmw);

  // map
  gmw->simple_map = shumate_simple_map_new();
  gmw->map_source_registry = shumate_map_source_registry_new_with_defaults();
  ShumateMapSource *ms = shumate_map_source_registry_get_by_id(
      gmw->map_source_registry, SHUMATE_MAP_SOURCE_OSM_MAPNIK);
  shumate_simple_map_set_map_source(gmw->simple_map, ms);

  ShumateViewport *vp = shumate_simple_map_get_viewport(gmw->simple_map);
  shumate_viewport_set_zoom_level(vp, 16.0);

  ShumateMap *map = shumate_simple_map_get_map(gmw->simple_map);
  shumate_map_center_on(map, 36.5519514, 31.9801362);
  // shumate_map_go_to_full(map, 36.5519514, 31.9801362, 14.0);

  gmw->map_marker_layer = shumate_marker_layer_new(vp);
  for (size_t i = 0; i < MC_COUNT; ++i) {
    gmw->map_path_layers[i] = shumate_path_layer_new(vp);
    shumate_simple_map_add_overlay_layer(
        gmw->simple_map, SHUMATE_LAYER(gmw->map_path_layers[i]));
  }

  shumate_simple_map_add_overlay_layer(gmw->simple_map,
                                       SHUMATE_LAYER(gmw->map_marker_layer));

  // todo move to gmv and free in destructor to avoid memory leak
  GtkGestureClick *ggc = GTK_GESTURE_CLICK(gtk_gesture_click_new());
  gtk_widget_add_controller(GTK_WIDGET(gmw->simple_map),
                            GTK_EVENT_CONTROLLER(ggc));
  g_signal_connect(ggc, "released",
                   G_CALLBACK(gmw_simple_map_gesture_click_released), gmw);

  // grid
  GtkWidget *grid = gtk_grid_new();
  gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
  gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
  gtk_widget_set_hexpand(GTK_WIDGET(grid), true);
  gtk_widget_set_hexpand(GTK_WIDGET(grid), true);

  gtk_grid_attach(GTK_GRID(grid), btn_save_trajectory, 0, 0, 1, 1);
  gtk_grid_attach(GTK_GRID(grid), btn_clear_all_points, 1, 0, 1, 1);
  gtk_grid_attach(GTK_GRID(grid), GTK_WIDGET(gmw->simple_map), 0, 1, 2, 2);

  // set grid as child of main window
  gtk_window_set_child(GTK_WINDOW(gmw->window), grid);
}
//////////////////////////////////////////////////////////////

void gmw_simple_map_gesture_click_released(GtkGestureClick *gesture,
                                           int n_press, double x, double y,
                                           gpointer user_data) {
  if (n_press > 1) return;  // do nothing
  GdkModifierType mt = gtk_event_controller_get_current_event_state(
      GTK_EVENT_CONTROLLER(gesture));
  if (!(mt & GDK_CONTROL_MASK)) return;  // add marker only with ctrl

  generator_main_window *gmw =
      reinterpret_cast<generator_main_window *>(user_data);
  ShumateViewport *vp = shumate_simple_map_get_viewport(gmw->simple_map);
  double lat, lng;
  shumate_viewport_widget_coords_to_location(vp, GTK_WIDGET(gmw->simple_map), x,
                                             y, &lat, &lng);
  gmw_add_marker(gmw, MC_RED, lat, lng);
  gmw->lst_geopoints.push_back(geopoint(lat, lng));
}
//////////////////////////////////////////////////////////////

void gmw_btn_clear_all_points_clicked(GtkWidget *btn, gpointer ud) {
  (void)btn;
  generator_main_window *gmw = reinterpret_cast<generator_main_window *>(ud);
  gmw->lst_geopoints.clear();
  for (int i = 0; i < MC_COUNT; ++i) {
    shumate_path_layer_remove_all(gmw->map_path_layers[i]);
    shumate_marker_layer_remove_all(gmw->map_marker_layer);
  }
}
//////////////////////////////////////////////////////////////

void gmw_btn_save_trajectory(GtkWidget *btn, gpointer ud) {
  (void)btn;
  generator_main_window *gmw = reinterpret_cast<generator_main_window *>(ud);

  for (auto gp : gmw->lst_geopoints) {
    std::cout << std::setprecision(12) << gp.latitude << " , " << gp.longitude << "\n";
  }
}
//////////////////////////////////////////////////////////////

void gmw_add_marker(generator_main_window *gmw, marker_color mc,
                    double latitude, double longitude) {
  map_marker_resource resources[] = {{.buff = map_marker_green_buff,
                                      .buff_len = map_marker_green_len,
                                      .width = map_marker_green_width,
                                      .heigth = map_marker_green_heigth},
                                     {.buff = map_marker_red_buff,
                                      .buff_len = map_marker_red_len,
                                      .width = map_marker_red_width,
                                      .heigth = map_marker_red_heigth},
                                     {.buff = map_marker_blue_buff,
                                      .buff_len = map_marker_blue_len,
                                      .width = map_marker_blue_width,
                                      .heigth = map_marker_blue_heigth}};

  GBytes *gbytes = g_bytes_new(resources[mc].buff, resources[mc].buff_len);
  GdkPixbuf *pb = gdk_pixbuf_new_from_bytes(
      gbytes, GDK_COLORSPACE_RGB, true, 8, resources[mc].width,
      resources[mc].heigth, resources[mc].width * 4);
  GtkWidget *img = gtk_image_new_from_pixbuf(pb);
  ShumateMarker *marker = shumate_marker_new();
  shumate_location_set_location(SHUMATE_LOCATION(marker), latitude, longitude);
  shumate_marker_set_child(marker, img);
  gtk_widget_set_size_request(GTK_WIDGET(marker), 48, 48);

  shumate_marker_layer_add_marker(gmw->map_marker_layer, marker);
  shumate_path_layer_add_node(gmw->map_path_layers[mc],
                              SHUMATE_LOCATION(marker));
}
//////////////////////////////////////////////////////////////
