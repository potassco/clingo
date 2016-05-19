weekend(sat).  weekend(sun).
weekdays :- day(X) : day(X) : not weekend(X).
