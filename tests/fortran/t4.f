C     CHARACTERS; AN INTERNAL FILE; WHAT IS READ FROM THE STANDARD
C     INPUT; AND FILES, FORMATTED, UNFORMATTED AND DIRECT
      PROGRAM T4
      CHARACTER*10 S, T
      CHARACTER*20 U
      CHARACTER*6 B
      INTEGER I, J, K, M, V(3), RV(3)
      REAL X, Y
      S = 'HELLO'
      T = 'WORLD'
      U = S(1:5)//', '//T(1:5)
      WRITE (6, 400) U(1:12), INDEX(U, 'WORLD'), LEN(U)
  400 FORMAT (' CONCAT ', A, 2I3)
      WRITE (6, 401) S .LT. T, LLT('ABC', 'ABD'), S .EQ. 'HELLO'
  401 FORMAT (' COMPARE', 3L2)
      WRITE (B, '(I6)') 4321
      READ (B, '(I6)') J
      WRITE (6, 402) B, J + 1
  402 FORMAT (' INTERNAL ', A, I6)
      READ (5, *) I, X
      READ (5, 403) T
  403 FORMAT (A)
      WRITE (6, 404) I*2, X*2.0, T(1:5)
  404 FORMAT (' INPUT', I5, F7.2, 1X, A)
      OPEN (7, FILE='t4.dat', STATUS='NEW')
      DO 10 K = 1, 3
         WRITE (7, 405) K, K*K
   10 CONTINUE
  405 FORMAT (2I4)
      REWIND 7
      M = 0
      DO 20 K = 1, 3
         READ (7, 405) I, J
         M = M + I*J
   20 CONTINUE
      CLOSE (7, STATUS='DELETE')
      WRITE (6, 406) M
  406 FORMAT (' FILE', I5)
      V(1) = 100000
      V(2) = -5
      V(3) = 7
      OPEN (8, FILE='t4.bin', FORM='UNFORMATTED', STATUS='NEW')
      WRITE (8) V, 2.5
      REWIND 8
      READ (8) RV, Y
      CLOSE (8, STATUS='DELETE')
      WRITE (6, 407) RV, Y
  407 FORMAT (' BINARY', 3I8, F5.1)
      OPEN (9, FILE='t4.dir', ACCESS='DIRECT', RECL=4, STATUS='NEW')
      DO 30 K = 1, 5
         WRITE (9, REC=K) K*11
   30 CONTINUE
      READ (9, REC=4) I
      READ (9, REC=2) J
      CLOSE (9, STATUS='DELETE')
      WRITE (6, 408) I, J
  408 FORMAT (' DIRECT', 2I4)
      PRINT *, 'LIST', 42
      END
