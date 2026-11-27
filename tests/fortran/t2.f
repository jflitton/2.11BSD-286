C     REAL, DOUBLE PRECISION, COMPLEX AND LOGICAL: CONSTANTS, DATA, A
C     STATEMENT FUNCTION, THE INTRINSIC FUNCTIONS AND POWERS
      PROGRAM T2
      REAL A(3), R, F, X
      DOUBLE PRECISION D, E
      COMPLEX C, Z
      INTEGER I, J, N, M
      LOGICAL L1, L2
      DATA A / 1.5, -2.25, 1.0E10 /, D / 3.141592653589793D0 /
      DATA N / 123456 /, M / -123456 /
      F(X) = X*X + 1.0
      R = A(1) + A(2)
      WRITE (6, 200) R, A(3)
  200 FORMAT (' REAL', F8.3, E12.4)
      D = D*2.0D0
      WRITE (6, 201) D
  201 FORMAT (' DOUBLE', F12.8)
      WRITE (6, 202) N, M, N + M
  202 FORMAT (' DATA', 3I8)
      WRITE (6, 203) SQRT(16.0), ABS(-3.5), F(3.0)
  203 FORMAT (' FUNCS', 3F6.2)
      WRITE (6, 204) MOD(17, 5), MAX(3, 9, 4), MIN(8, -2), INT(7.9),
     1   NINT(7.5), IABS(-12)
  204 FORMAT (' INTS', 6I4)
      E = DATAN(1.0D0)*4.0D0
      WRITE (6, 205) E, DSQRT(2.0D0)
  205 FORMAT (' PI', F11.7, F11.7)
      I = 3
      J = 2
      WRITE (6, 206) I**15, J**I, 2.0**I, 2.0D0**0.5D0
  206 FORMAT (' POWERS', I10, I4, F6.1, F11.7)
      C = (1.0, 2.0)
      Z = C*C
      WRITE (6, 207) Z, C/(0.0, 1.0), CABS((3.0, 4.0))
  207 FORMAT (' COMPLEX', 4F6.2, F6.2)
      L1 = .TRUE.
      L2 = .NOT. L1 .OR. 3 .GT. 2
      WRITE (6, 208) L1, L2, L1 .AND. .NOT. L2
  208 FORMAT (' LOGICAL', 3L2)
      END
