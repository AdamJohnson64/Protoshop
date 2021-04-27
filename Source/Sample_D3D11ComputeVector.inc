// Although you can adjust these dimensions it is important to remember that
// the taller a threadgroup is the more edges it will cross. It is desirable
// to process many X pixels at once but only few rasters.

#define THREADCOUNT_X 256
#define THREADCOUNT_Y 1