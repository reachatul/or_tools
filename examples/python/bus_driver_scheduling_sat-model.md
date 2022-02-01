# Driver's Sceduling

## Sets:

$D$ - Set of all the drivers$ \in 0..d$

$T$ - Set of all the trucks $\in 0..t$

$S$ - Set of all the shifts $\in 0..o$

## Variables:

$startTime_{d} \in Z$  

Start time of a driver.

$endTime_d \in Z$

End time of a driver.

$drivingTime_d \in Z$

$workingTime_d \in Z$

$performed_{d, s} \in (0, 1)$

 $noBreakDriving \in Z$

$totalDriving \in Z$

$sourceLiteral_{d, s}$ 

$sinkLiteral_{d, s}$

$sequenceLiteral_{d, s, o}$

## Parameters

$startTime_{s}$

$duration_s$

$cleanUpTime$

## Constraints:

### Source Constraints

1. Start time of a driver should be equal to the first shift he is taking, (from source.)

$$
startTime_d = startTime_{s}  \ \iff sourceLiteral_{d, s}\ \ \forall d, s

$$

2. Driving Time of a driver for a shift should be equal to the total duration of that shift
   
   $$
   totalDriving_{d, s} = duration_s  \ \ \iff sourceLiteral_{d, s}\ \ \forall d, s

   $$

3. No Break Driving:
   
   $$
   noBreakDriving_{d, s} = duration_s  \ \ \iff sourceLiteral_{d, s}\ \ \forall d, s
   $$

### Sink Constraints

1. End time of a driver should be equal to the last shift he is taking, (from source.)

$$
endTime_d = endTime_{s} + cleanUpTime  \ \iff sinkLiteral_{d, s} \ \ \forall d, s

$$

2. Driving Times of a driver for a shift should be equal to the total duration of that shift

$$
drivingTimes_{d} = totalDriving_{d, s}  \ \ \iff sinkLiteral_{d, s} \ \ \forall d, s


$$

### Node Not Performed

 These constraints are active is the node is not performed $~perfromed_{d, s}$

1. Total driving and no break driving should be zero is for performed.

$$
totalDriving_{d, s} = 0  \iff \neg performed_{d, s} \ \ \forall d, s 


$$

$$
noBreakDriving_{d, s} = 0 \iff \ \ \neg performed_{d, s}\ \ \forall d, s


$$

### Node Performed

This constrain gives upper and lower bound for the startTime and endTime of the driver.

$$
startTime_{d, s} \leq start_s \iff performed_{d, s}\ \ \forall d, s \ 

$$

$$
endTime_{d, s} \geq end_s m \ \iff perfromed_{d, s} \ \ \forall d, s
$$



### Sequence Based Constraints

Here every shift is compared with every other shift.

$$
totalDriving_{d, o} = totalDriving_{d, s} + duration_o \ \ \forall \  s, o \in S d
$$

No break driving

if $start_o - end_s > allowedDealy$

$$
noBreakDriving_{d, o} = duration_o \iff sequenceLiteral_{d, s, o} 
$$

else:

$$
noBreakDriving_{d, o} = noBreakDriving_{d, s} +duration_o \iff sequenceLiteral_{d, s, o}
$$



### Circuit Constraints

A driver can have only one starting shift if he is performing that shift.

$$
\sum_{s}sourceLiterals_{d, s} = 1 \ \ \forall d
$$

$$
\sum_{s}sinkLiterals_{d, s} = 1 \ \ \forall d
$$

$$
sinkLiteral_{d, s} + sequenceLiteral_{d, s, o} + \neg performed_{d, s} = 1 \ \ \forall d, s
$$

$$
sourceLiteral_{d, s} + sequencLiteral_{d, o, s} + \neg performed_{d, s} = 1 \ \ forall \  d, s
$$




