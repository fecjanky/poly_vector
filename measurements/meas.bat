@ECHO OFF
FOR /L %%I IN (0,1,9) DO ( 
START "" /B /W /REALTIME  /AFFINITY 2 "%1" %2 %3 %4 %5
)