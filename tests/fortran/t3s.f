C     WHAT T3 CALLS
      SUBROUTINE ADD(A, N, T)
      REAL A(N), T
      T = 0.0
      DO 10 I = 1, N
         T = T + A(I)
   10 CONTINUE
      RETURN
      END
C
C     TWO ENTRIES WITH THEIR ARGUMENTS IN DIFFERENT PLACES
      INTEGER FUNCTION ISUM(N)
      INTEGER TWICE, M
      ISUM = 0
      DO 10 I = 1, N
         ISUM = ISUM + I
   10 CONTINUE
      RETURN
      ENTRY TWICE(M)
      TWICE = 2*M
      RETURN
      END
      SUBROUTINE PAIR(A, B)
      INTEGER A, B
      A = 1
      B = 2
      RETURN
      ENTRY SECOND(B)
      B = B + 40
      RETURN
      END
      DOUBLE PRECISION FUNCTION HALF(X)
      DOUBLE PRECISION X
      HALF = X/2.0D0
      RETURN
      END
      REAL FUNCTION APPLY(F, X)
      EXTERNAL F
      APPLY = F(X)
      RETURN
      END
      REAL FUNCTION SQ(X)
      SQ = X*X
      RETURN
      END
      COMPLEX FUNCTION SWAP(Z)
      COMPLEX Z
      SWAP = CMPLX(AIMAG(Z), REAL(Z))
      RETURN
      END
      CHARACTER*5 FUNCTION REV(S)
      CHARACTER*5 S
      DO 10 I = 1, 5
         REV(I:I) = S(6-I:6-I)
   10 CONTINUE
      RETURN
      END
      INTEGER FUNCTION LENGTH(S)
      CHARACTER*(*) S
      LENGTH = LEN(S)
      RETURN
      END
      LOGICAL FUNCTION ODD(N)
      ODD = MOD(N, 2) .EQ. 1
      RETURN
      END
      SUBROUTINE FILL(N)
      COMMON /WORK/ WK(100)
      DO 10 I = 1, N
         WK(I) = I
   10 CONTINUE
      RETURN
      END
      BLOCK DATA
      COMMON /BLK/ NC, CC(3)
      DATA NC / 3 /, CC / 1.5, 2.5, 3.5 /
      END
