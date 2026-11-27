C     CONTROL: THE ARITHMETIC IF ON EACH TYPE, THE ASSIGNED AND THE
C     COMPUTED GO TO, DO AND THE BLOCK IF; AND INTEGERS OF TWO BYTES
C     AND OF FOUR
      PROGRAM T1
      INTEGER*2 I2, J2
      INTEGER I4, J4, K, L
      REAL R
      DOUBLE PRECISION D
      I2 = 300
      I2 = I2*100 + 7
      WRITE (6, 100) I2
  100 FORMAT (' SHORT', I7)
      I4 = 100000
      J4 = -70000
      I4 = I4*3 + J4/7
      WRITE (6, 101) I4, MOD(I4, 7777)
  101 FORMAT (' LONG', I8, I6)
      K = 0
      DO 10 I = 1, 10
         K = K + I*I
   10 CONTINUE
      WRITE (6, 102) K
  102 FORMAT (' SQUARES', I5)
C
C     A LONG BELOW ZERO WHOSE LOW WORD IS NOT, AND ONE ABOVE WHOSE LOW
C     WORD IS
      DO 20 J4 = -1, 1
         IF (J4*100000) 11, 12, 13
   11    WRITE (6, 103) J4, 'BELOW'
         GO TO 20
   12    WRITE (6, 103) J4, 'ZERO'
         GO TO 20
   13    WRITE (6, 103) J4, 'ABOVE'
   20 CONTINUE
  103 FORMAT (' LONG IF', I3, 1X, A)
      DO 30 J2 = -1, 1
         IF (J2) 21, 22, 23
   21    WRITE (6, 104) J2, 'BELOW'
         GO TO 30
   22    WRITE (6, 104) J2, 'ZERO'
         GO TO 30
   23    WRITE (6, 104) J2, 'ABOVE'
   30 CONTINUE
  104 FORMAT (' SHORT IF', I3, 1X, A)
      DO 40 K = 1, 3
         R = K*1.5 - 3.0
         IF (R) 31, 32, 33
   31    WRITE (6, 105) K, 'BELOW'
         GO TO 40
   32    WRITE (6, 105) K, 'ZERO'
         GO TO 40
   33    WRITE (6, 105) K, 'ABOVE'
   40 CONTINUE
  105 FORMAT (' REAL IF', I2, 1X, A)
C
C     TOO SMALL FOR A REAL
      DO 50 K = 1, 3
         D = (K - 2)*1.0D-300
         IF (D) 41, 42, 43
   41    WRITE (6, 106) K, 'BELOW'
         GO TO 50
   42    WRITE (6, 106) K, 'ZERO'
         GO TO 50
   43    WRITE (6, 106) K, 'ABOVE'
   50 CONTINUE
  106 FORMAT (' DOUBLE IF', I2, 1X, A)
      ASSIGN 51 TO L
      GO TO 60
   51 WRITE (6, 107) 'FIRST'
      ASSIGN 52 TO L
      GO TO 60
   52 WRITE (6, 107) 'SECOND'
      GO TO 70
   60 GO TO L, (51, 52)
   70 CONTINUE
  107 FORMAT (' ASSIGNED ', A)
      DO 80 K = 1, 3
         GO TO (71, 72, 73), K
   71    WRITE (6, 108) K, 'ONE'
         GO TO 80
   72    WRITE (6, 108) K, 'TWO'
         GO TO 80
   73    WRITE (6, 108) K, 'THREE'
   80 CONTINUE
  108 FORMAT (' COMPUTED', I2, 1X, A)
      DO 90 K = 1, 3
         IF (K .EQ. 1) THEN
            WRITE (6, 109) K, 'FIRST'
         ELSE IF (K .EQ. 2) THEN
            WRITE (6, 109) K, 'SECOND'
         ELSE
            WRITE (6, 109) K, 'THIRD'
         END IF
   90 CONTINUE
  109 FORMAT (' BLOCK IF', I2, 1X, A)
      K = 0
      DO 95 I = 10, 1, -3
         K = K*10 + I
   95 CONTINUE
      WRITE (6, 110) K
  110 FORMAT (' DOWN', I6)
      END
