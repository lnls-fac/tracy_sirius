!     PARAMETERS:
!
!     LDA: MAXIMUM NUMBER OF DA-VECTORS;    CAN BE CHANGED QUITE ARBITRARILY
!     LST: LENGTH OF MAIN STORAGE STACK;    CAN BE CHANGED QUITE ARBITRARILY
!     LEA: MAXIMUM NUMBER OF MONOMIALS;     CAN BE INCREASED FOR LARGE NO, NV
!     LIA: DIMENSION OF IA1, IA2;           CAN BE INCREASED FOR LARGE NO, NV
!     LNO: MAXIMUM ORDER;                   CAN BE INCREASED TO ABOUT 1000
!     LNV: MAXIMUM NUMBER OF VARIABLES;     CAN BE INCREASED TO ABOUT 1000
!
!-----------------------------------------------------------------------------1

      integer           lda, lea, lia, lno, lnv, lst

      parameter (lda=30000, lst=200000000, lea=500000, lia=80000,         &
     &           lno=16, lnv=7)

      integer           nda, ndamaxi
      common /fordes/   nda, ndamaxi

      double precision  cc, eps, epsmac
      common /da/       cc(lst), eps, epsmac

      integer           i1, i2, ie1, ie2, ieo
      integer           ia1, ia2, ifi, idano
      integer           idanv, idapo, idalm, idall
      integer           nst, nomax, nvmax, nmmax, nocut, lfi
      common /dai/      i1(lst), i2(lst), ie1(lea), ie2(lea), ieo(lea), &
     &                  ia1(0:lia), ia2(0:lia), ifi(lea), idano(lda),   &
     &                  idanv(lda), idapo(lda), idalm(lda), idall(lda), &
     &                  nst, nomax, nvmax, nmmax, nocut, lfi

      double precision  facint
      common /factor/   facint(0:lno)
!-----------------------------------------------------------------------------9
