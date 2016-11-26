__kernel void kernel_acc_pop(__global float* lat, __global float *longi, __global int *pop,
							__global int *acc_pop, int no_cities, int kmrange)
{
	uint gid = get_global_id(0);

	/* constants */
	float MIN_LAT = -M_PI / 2, MAX_LAT = M_PI / 2;
	float MIN_LON = -2 * M_PI, MAX_LON = 2 * M_PI;
	float EARTH_R = 6371;
	float DTR = 0.0174533 /*degrres to radians*/, RTD = 57.295827 /*radians to degrees*/;

	float r, my_lat, my_lon;
	float lon_min, lon_max, delta_lon, lat_min, lat_max;
	float sin_phi1, cos_phi1, phi2, theta1, theta2, dist;
	
	r = kmrange / EARTH_R;
	/* transform degrees in radians */
	my_lat = lat[gid] * DTR;
	my_lon = longi[gid] * DTR;

	/* compute min and max latitude */
	lat_min = my_lat - r;
	lat_max = my_lat + r;

	/* compute min and max longitude */
	if (lat_min > MIN_LAT && lat_max < MAX_LAT) {
		delta_lon = asin(sin(r) / cos(my_lat));

		lon_min = my_lon - delta_lon;
		if (lon_min < MIN_LON)
			lon_min += 2 * M_PI;

		lon_max = my_lon + delta_lon;
		if (lon_max > MAX_LON)
			lon_max -= 2 * M_PI;
	} else {
		lat_min = max(lat_min, MIN_LAT);
		lat_max = min(lat_max, MAX_LAT);
		lon_min = MIN_LON;
		lon_max = MAX_LON;
	}

	/* transform back to degrees */
	lat_min *= RTD;
	lat_max *= RTD;
	lon_min *= RTD;
	lon_max *= RTD;

	
	sin_phi1 = sin((90 - lat[gid]) * DTR);
	cos_phi1 = cos((90 - lat[gid]) * DTR);
	theta1 = my_lon;

	acc_pop[gid] = pop[gid];

	/* check each city if it's between the min and max coordinates */
	for (int i = 0; i < no_cities; i++) {
		if (lat[i] >= lat_min && lat[i] <= lat_max &&
			longi[i] >= lon_min && longi[i] <= lon_max && i != gid) {
			/* if it is, compute the distance */
			phi2 = (90 - lat[i]) * DTR;
			theta2 = longi[i] * DTR;

			dist = acos(sin_phi1 * sin(phi2) * cos(theta1 - theta2) +
						cos_phi1 * cos(phi2)) * EARTH_R;

			/* if distance is accepted, add to acc population */
			if (dist <= kmrange) {
				acc_pop[gid] += pop[i];
			}
		}
	}
}
