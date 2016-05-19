course(1,1,5). course(1,2,5).
course(2,1,4). course(2,2,4).
course(3,1,6).                course(3,3,6).
course(4,1,3).                course(4,3,3). course(4,4,3).
course(5,1,4).                               course(5,4,4).
               course(6,2,2). course(6,3,2).
               course(7,2,4). course(7,3,4). course(7,4,4).
                              course(8,3,5). course(8,4,5).


3 { enroll(C) : course(C,S,H) } 6.

:- [ enroll(C) : course(C,S,H) ] 10.
:- 2 [ not enroll(C) : course(C,2,H) ].
:- 6 [ enroll(C) : course(C,3,H), enroll(C) : course(C,4,H) ].

hours(C,H) :- course(C,S,H).
max_hours(20).

:- not M-2 [ enroll(C) : hours(C,H) = H ] M, max_hours(M).
:- min [ enroll(C) : hours(C,H) = H ] 2.
:- 6 max [ enroll(C) : hours(C,H) = H ].

courses(N) :- N = count { enroll(C) : course(C,S,H) }.
hours(N)   :- N = sum [ enroll(C) : hours(C,H) = H ].


#hide.
#show enroll/1.
#show courses/1.
#show hours/1.
