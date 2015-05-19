/*
EquaRes - a tool for numerical analysis of equations.
Copyright (C) 2015 Stepan Orlov

This file is part of EquaRes.

Full text of copyright notice can be found in file copyright_notice.txt in the EquaRes root directory.
License agreement can be found in file LICENSE.md in the EquaRes root directory.
*/

#ifndef FSMX_H
#define FSMX_H

#include <assert.h>
#include <algorithm>
#include <cmath>

#ifdef _DEBUG
#define ASSERT(x) assert(x)
#else
#define ASSERT(x)
#endif

namespace ctm { namespace cxx {
template< bool X > struct CompileAssert {};
    } }

#define COMPILE_ASSERT(expr) \
    typedef ::ctm::cxx::CompileAssert<(bool(expr))> CompileAssertTypedef[bool(expr) ? 1 : -1]

namespace fsmx {

template< class DL, class DR, class F >
void memberwise(DL& left, const DR& right, F func)
{
    COMPILE_ASSERT(static_cast<int>(DL::Nr) == DR::Nr && static_cast<int>(DL::Nc) == DR::Nc);
    typedef typename DL::value_type T;
    T *pLeftRow = left.data();
    const T *pRightRow = right.data();
    for (int r=0; r<DL::Nr; ++r, pLeftRow+=DL::Rs, pRightRow+=DR::Rs)
    {
        T *pLeft = pLeftRow;
        const T *pRight = pRightRow;
        for (int c=0; c<DL::Nc; ++c, pLeft+=DL::Cs, pRight+=DR::Cs)
            func(*pLeft, *pRight);
    }
}

template< class D, class F >
void memberwise(D& x, F func)
{
    typedef typename D::value_type T;
    T *pRow = x.data();
    for (int r=0; r<D::Nr; ++r, pRow+=D::Rs)
    {
        T *p = pRow;
        for (int c=0; c<D::Nc; ++c, p+=D::Cs)
            func(*p);
    }
}

template< class T > inline void assign(T& left, const T& right) { left = right; }
template< class T > inline void addAssign(T& left, const T& right) { left += right; }
template< class T > inline void subAssign(T& left, const T& right) { left -= right; }
template< class T > inline void mulAssign(T& left, const T& right) { left *= right; }

template< class T >
class scaleAssign {
public:
    explicit scaleAssign(const T& factor) : m_factor(factor) {}
    void operator()(T& x) const {
        x *= m_factor;
    }
private:
    T m_factor;
};

template< class T >
class setValue {
public:
    explicit setValue(const T& value) : m_value(value) {}
    void operator()(T& x) const {
        x = m_value;
    }
private:
    T m_value;
};

template< class T >
class setValues {
public:
    explicit setValues(const T *values) : m_values(values) {}
    void operator()(T& x) const {
        x = *m_values++;
    }
private:
    mutable const T *m_values;
};

template< class T > inline void negAssign(T& x) { x = -x; }

template< int nr, int nc, class T = double >
class Data
{
public:
    typedef T value_type;
    typedef T accessed_value_type;
    typedef Data<nr,nc,T> ThisClass;
    enum { Nr = nr, Nc = nc, Rs = nc, Cs = 1, Length = nr*nc };
    typedef const ThisClass& tmp_clone_type;

    Data() {}
    Data(const ThisClass& r) {
        std::copy(r.data(), r.data() + Length, m_data);
    }
    template< class D >
    Data(const D& x) {
        memberwise(*this, x, assign<T>);
    }
    template< class D >
    Data& operator=(const D& x) {
        memberwise(*this, x, assign<T>);
        return *this;
    }
    Data& operator=(const ThisClass& x) {
        if (this != &x)
            memberwise(*this, x, assign<T>);
        return *this;
    }

    T& operator()(int r, int c)
    {
        ASSERT(r >= 0 && r < nr);
        ASSERT(c >= 0 && c < nc);
        return m_data[r*nc+c];
    }
    const T& operator()(int r, int c) const {
        return (*const_cast<ThisClass*>(this))(r, c);
    }
    T *data() { return m_data; }
    const T *data() const { return m_data; }

    void fill(const T& x) {
        std::fill(m_data, m_data + Length, x);
    }
    void set(const T *x) {
        std::copy(x, x+Length, m_data);
    }

private:
    T m_data[Length];
};

template< class D >
class TransposeProxy
{
public:
    typedef typename D::value_type value_type;
    typedef const value_type accessed_value_type;
    enum { Nr = D::Nc, Nc = D::Nr, Rs = D::Cs, Cs = D::Rs, Length = D::Length };
    typedef const Data<Nr, Nc, value_type> tmp_clone_type;

    explicit TransposeProxy(const D& src) : m_src(src) {}
    const value_type& operator()(int r, int c) const {
        return m_src(c, r);
    }
    const value_type *data() const { return m_src.data(); }
private:
    const D& m_src;
};

template< int nr, int nc, int rs, int cs, class T, class T2 >
class SliceProxy
{
public:
    typedef T value_type;
    typedef T2 accessed_value_type;
    typedef SliceProxy<nr, nc, rs, cs, T, T2> ThisClass;
    enum { Nr = nr, Nc = nc, Rs = rs, Cs = cs, Length = nr*nc };
    typedef const Data<Nr, Nc, value_type> tmp_clone_type;

    explicit SliceProxy(T2 *data) : m_data(data) {}

    template<class D>
    ThisClass& operator=(const D& x) {
        memberwise(*this, x, assign<T>);
        return *this;
    }
    ThisClass& operator=(const ThisClass& x) {
        if (this != &x)
            memberwise(*this, x, assign<T>);
        return *this;
    }

    accessed_value_type& operator()(int r, int c) const {
        ASSERT(r >= 0 && r < nr);
        ASSERT(c >= 0 && c < nc);
        return m_data[r*rs+c*cs];
    }
    accessed_value_type *data() const { return m_data; }

    void fill(const T& x) const {
        memberwise(*this, setValue<T>(x));
    }
    void set(const T *x) const {
        memberwise(*this, setValues<T>(x));
    }
private:
    accessed_value_type *m_data;
};

template< class D >
class MX
{
public:
    typedef D Data;
    typedef TransposeProxy<D> TransposedData;
    typedef typename D::value_type value_type;
    typedef MX<D> ThisClass;
    enum { Nr = D::Nr, Nc = D::Nc, Rs = D::Rs, Cs = D::Cs, Length = D::Length };
    typedef SliceProxy<Nr, 1, Rs, 0, value_type, value_type> CD;
    typedef MX<CD> Column;
    typedef SliceProxy<Nr, 1, Rs, 0, value_type, const value_type> CCD;
    typedef MX<CCD> ConstColumn;
    typedef SliceProxy<1, Nc, 0, Cs, value_type, value_type> RD;
    typedef MX<RD> Row;
    typedef SliceProxy<1, Nc, 0, Cs, value_type, const value_type> CRD;
    typedef MX<CRD> ConstRow;
    typedef SliceProxy<Nr, 1, Rs+Cs, 0, value_type, value_type> DCD;
    typedef MX<DCD> DiagAsColumn;
    typedef SliceProxy<Nr, 1, Rs+Cs, 0, value_type, const value_type> CDCD;
    typedef MX<CDCD> ConstDiagAsColumn;
    typedef SliceProxy<Nr, 1, Rs-Cs, 0, value_type, value_type> D2CD;
    typedef MX<D2CD> Diag2AsColumn;
    typedef SliceProxy<Nr, 1, Rs-Cs, 0, value_type, const value_type> CD2CD;
    typedef MX<CD2CD> ConstDiag2AsColumn;
    MX() {}
    MX(const ThisClass& r) : m_data(r.data()) {}
    template< class DR >
    MX(const MX<DR>& r) : m_data(r.data()) {}
    template< class DR >
    MX(const DR& r) : m_data(r) {}

    template< class DR >
    ThisClass& operator=(const MX<DR>& r) {
        m_data = r.tmpData();
        return *this;
    }

    D& data() { return m_data; }
    const D& data() const { return m_data; }
    typename D::tmp_clone_type tmpData() const { return m_data; }
    value_type& operator()(int r, int c) { return m_data(r, c); }
    const value_type& operator()(int r, int c) const { return m_data(r, c); }
    Column col(int c) {
        ASSERT(c >= 0   &&   c < Nc);
        return Column(CD(m_data.data() + c*Cs));
        }
    ConstColumn constCol(int c) const {
        ASSERT(c >= 0   &&   c < Nc);
        return ConstColumn(CCD(m_data.data() + c*Cs));
        }
    ConstColumn col(int c) const {
        return constCol(c);
        }
    Row row(int r) {
        ASSERT(r >= 0   &&   r < Nr);
        return Row(RD(m_data.data() + r*Rs));
        }
    ConstRow constRow(int r) const {
        ASSERT(r >= 0   &&   r < Nr);
        return ConstRow(CRD(m_data.data() + r*Rs));
        }
    ConstRow row(int r) const {
        return constRow(r);
        }

    DiagAsColumn diag() {
        COMPILE_ASSERT(Nr == Nc);
        return DiagAsColumn(DCD(m_data.data()));
        }
    ConstDiagAsColumn constDiag() const {
        COMPILE_ASSERT(Nr == Nc);
        return ConstDiagAsColumn(CDCD(m_data.data()));
        }
    ConstDiagAsColumn diag() const {
        return constDiag();
        }

    template< int n >
    MX< SliceProxy<Nr-n, 1, Rs+Cs, 0, value_type, value_type> > ldiag() {
        COMPILE_ASSERT(Nr == Nc   &&   n >= 0   &&   n < Nr);
        typedef SliceProxy<Nr-n, 1, Rs+Cs, 0, value_type, value_type> NDCD;
        return MX<NDCD>(NDCD(m_data.data() + n*Rs));
        }
    template< int n >
    MX< SliceProxy<Nr-n, 1, Rs+Cs, 0, value_type, const value_type> > constLdiag() const {
        COMPILE_ASSERT(Nr == Nc   &&   n >= 0   &&   n < Nr);
        typedef SliceProxy<Nr-n, 1, Rs+Cs, 0, value_type, const value_type> CNDCD;
        return MX<CNDCD>(CNDCD(m_data.data() + n*Rs));
        }
    template< int n >
    MX< SliceProxy<Nr-n, 1, Rs+Cs, 0, value_type, const value_type> > ldiag() const {
        return constLdiag<n>();
        }
    template< int n >
    MX< SliceProxy<Nr-n, 1, Rs+Cs, 0, value_type, value_type> > rdiag() {
        COMPILE_ASSERT(Nr == Nc   &&   n >= 0   &&   n < Nr);
        typedef SliceProxy<Nr-n, 1, Rs+Cs, 0, value_type, value_type> NDCD;
        return MX<NDCD>(NDCD(m_data.data() + n*Cs));
        }
    template< int n >
    MX< SliceProxy<Nr-n, 1, Rs+Cs, 0, value_type, const value_type> > constRdiag() const {
        COMPILE_ASSERT(Nr == Nc   &&   n >= 0   &&   n < Nr);
        typedef SliceProxy<Nr-n, 1, Rs+Cs, 0, value_type, const value_type> CNDCD;
        return MX<CNDCD>(CNDCD(m_data.data() + n*Cs));
        }
    template< int n >
    MX< SliceProxy<Nr-n, 1, Rs+Cs, 0, value_type, const value_type> > rdiag() const {
        return constRdiag<n>();
        }

    Diag2AsColumn diag2() {
        COMPILE_ASSERT(Nr == Nc);
        return Diag2AsColumn(D2CD(m_data.data() + (Nc-1)*Cs));
        }
    ConstDiag2AsColumn constDiag2() const {
        COMPILE_ASSERT(Nr == Nc);
        return ConstDiag2AsColumn(CD2CD(m_data.data() + (Nc-1)*Cs));
        }
    ConstDiag2AsColumn diag2() const {
        return constDiag2();
        }

    template< int n >
    MX< SliceProxy<Nr-n, 1, Rs-Cs, 0, value_type, value_type> > ldiag2() {
        COMPILE_ASSERT(Nr == Nc   &&   n >= 0   &&   n < Nr);
        typedef SliceProxy<Nr-n, 1, Rs-Cs, 0, value_type, value_type> NDCD;
        return MX<NDCD>(NDCD(m_data.data() + (Nc-1-n)*Cs));
        }
    template< int n >
    MX< SliceProxy<Nr-n, 1, Rs-Cs, 0, value_type, const value_type> > constLdiag2() const {
        COMPILE_ASSERT(Nr == Nc   &&   n >= 0   &&   n < Nr);
        typedef SliceProxy<Nr-n, 1, Rs-Cs, 0, value_type, const value_type> CNDCD;
        return MX<CNDCD>(CNDCD(m_data.data() + (Nc-1-n)*Cs));
        }
    template< int n >
    MX< SliceProxy<Nr-n, 1, Rs-Cs, 0, value_type, const value_type> > ldiag2() const {
        return constLdiag2<n>();
    }
    template< int n >
    MX< SliceProxy<Nr-n, 1, Rs-Cs, 0, value_type, value_type> > rdiag2() {
        COMPILE_ASSERT(Nr == Nc   &&   n >= 0   &&   n < Nr);
        typedef SliceProxy<Nr-n, 1, Rs-Cs, 0, value_type, value_type> NDCD;
        return MX<NDCD>(NDCD(m_data.data() + (Nc-1)*Cs + n*Rs));
        }
    template< int n >
    MX< SliceProxy<Nr-n, 1, Rs-Cs, 0, value_type, const value_type> > constRdiag2() const {
        COMPILE_ASSERT(Nr == Nc   &&   n >= 0   &&   n < Nr);
        typedef SliceProxy<Nr-n, 1, Rs-Cs, 0, value_type, const value_type> CNDCD;
        return MX<CNDCD>(CNDCD(m_data.data() + (Nc-1)*Cs + n*Rs));
        }
    template< int n >
    MX< SliceProxy<Nr-n, 1, Rs-Cs, 0, value_type, const value_type> > rdiag2() const {
        return constRdiag2<n>();
    }

    template< int nr, int nc >
    MX< SliceProxy<nr, nc, Rs, Cs, value_type, value_type> > block(int r, int c) {
        ASSERT(nr > 0   &&   r >= 0   &&   nr+r <= Nr);
        ASSERT(nc > 0   &&   c >= 0   &&   nc+c <= Nc);
        typedef SliceProxy<nr, nc, Rs, Cs, value_type, value_type> DB;
        return MX<DB>(DB(m_data.data() + r*Rs + c*Cs));
        }
    template< int nr, int nc >
    MX< SliceProxy<nr, nc, Rs, Cs, value_type, const value_type> > constBlock(int r, int c) const {
        ASSERT(nr > 0   &&   r >= 0   &&   nr+r <= Nr);
        ASSERT(nc > 0   &&   c >= 0   &&   nc+c <= Nc);
        typedef SliceProxy<nr, nc, Rs, Cs, value_type, const value_type> CDB;
        return MX<CDB>(CDB(m_data.data() + r*Rs + c*Cs));
        }
    template< int nr, int nc >
    MX< SliceProxy<nr, nc, Rs, Cs, value_type, const value_type> > block(int r, int c) const {
        return constBlock<nr,nc>(r,c);
        }


    MX< TransposedData > T() const {
        return MX< TransposedData >(m_data);
    }

    void fill(const value_type& x) {
        m_data.fill(x);
    }
    void fill(const value_type& x) const {
        m_data.fill(x);
    }
    static ThisClass filled(const value_type& x) {
        ThisClass result;
        result.fill(x);
        return result;
    }
    void set(const value_type *x) {
        m_data.set(x);
    }
    void set(const value_type *x) const {
        m_data.set(x);
    }
    static ThisClass zeros() { return filled(0); }
    static ThisClass ones() { return filled(1); }

    template< class DR >
    ThisClass& operator+=(const MX<DR>& r) {
        memberwise(m_data, r.tmpData(), addAssign<value_type>);
        return *this;
    }
    template< class DR >
    ThisClass& operator-=(const MX<DR>& r) {
        memberwise(m_data, r.tmpData(), subAssign<value_type>);
        return *this;
    }
    template< class DR >
    ThisClass& operator*=(const MX<DR>& r) {
        *this = *this * r;
        return *this;
    }
    ThisClass& operator*=(const value_type& r) {
        memberwise(m_data, scaleAssign<value_type>(r));
        return *this;
    }
    ThisClass& operator/=(const value_type& r) {
        memberwise(m_data, scaleAssign<value_type>(1/r));
        return *this;
    }
    MX< fsmx::Data<Nr, Nc, value_type> > operator-() const {
        MX< fsmx::Data<Nr, Nc, value_type> > result = *this;
        memberwise(result.data(), negAssign<value_type>);
        return result;
        }

    typename D::accessed_value_type& scalar() {
        COMPILE_ASSERT(Length == 1);
        return *m_data.data();
    }
    const value_type& scalar() const {
        COMPILE_ASSERT(Length == 1);
        return *m_data.data();
    }
    operator typename D::accessed_value_type& () { return scalar(); }
    operator const value_type& () const { return scalar(); }

    inline MX< fsmx::Data<D::Nr, D::Nc, typename D::value_type> > blockInv() const;
    inline MX< fsmx::Data<D::Nr, D::Nc, typename D::value_type> > inv() const;

    template< class D2 >
    inline MX< fsmx::Data<D::Nr, D2::Nc, typename D::value_type> > gaussSolve(const MX<D2>& f) const;

private:
    D m_data;
};

template< class D1, class D2 >
inline MX< Data<D1::Nr, D1::Nc, typename D1::value_type> > operator+(const MX<D1>& l, const MX<D2>& r)
{
    MX< Data<D1::Nr, D1::Nc, typename D1::value_type> > result = l;
    result += r;
    return result;
}

template< class D1, class D2 >
inline MX< Data<D1::Nr, D1::Nc, typename D1::value_type> > operator-(const MX<D1>& l, const MX<D2>& r)
{
    MX< Data<D1::Nr, D1::Nc, typename D1::value_type> > result = l;
    result -= r;
    return result;
}

template< class D1, class D2 >
inline MX< Data<D1::Nr, D2::Nc, typename D1::value_type> > operator*(const MX<D1>& l, const MX<D2>& r)
{
    typedef typename D1::value_type T1;
    typedef typename D2::value_type T2;
    typedef Data<D1::Nr, D2::Nc, T1> D;
    typedef MX<D> M;
    COMPILE_ASSERT(static_cast<int>(D1::Nc) == D2::Nr);
    enum { Nx = D1::Nc };
    M result;
    T1 *pRow = result.data().data();
    const T1 *pRowLeft = l.data().data();
    for (int ir=0; ir<M::Nr; ++ir, pRow+=M::Rs, pRowLeft+=D1::Rs) {
        T1 *p = pRow;
        const T2 *pColRight = r.data().data();
        for (int ic=0; ic<M::Nc; ++ic, p+=M::Cs, pColRight+=D2::Cs) {
            *p = 0;
            const T1 *pLeft = pRowLeft;
            const T2 *pRight = pColRight;
            for (int ix=0; ix<Nx; ++ix, pLeft+=D1::Cs, pRight+=D2::Rs)
                *p += *pLeft * *pRight;
        }
    }
    return result;
}

template< class D >
inline MX< fsmx::Data<D::Nr, D::Nc, typename D::value_type> > operator*(const MX<D>& l, const typename D::value_type& r) {
    MX< fsmx::Data<D::Nr, D::Nc, typename D::value_type> > result = l;
    result *= r;
    return result;
}

template< class D >
inline MX< fsmx::Data<D::Nr, D::Nc, typename D::value_type> > operator*(const typename D::value_type& l, const MX<D>& r) {
    MX< fsmx::Data<D::Nr, D::Nc, typename D::value_type> > result = r;
    result *= l;
    return result;
}

template< class D >
inline MX< fsmx::Data<D::Nr, D::Nc, typename D::value_type> > operator/(const MX<D>& l, const typename D::value_type& r) {
    MX< fsmx::Data<D::Nr, D::Nc, typename D::value_type> > result = l;
    result /= r;
    return result;
}

template< int n >
class BlockInv
    {
    public:
        enum { N = n, N1 = n>>1, N2 = n-N1 };
        template<class D>
        static inline MX< Data<N, N, typename D::value_type> > blockInv(const MX<D>& m) {
            typedef MX<D> M;
            typedef typename M::value_type T;
            COMPILE_ASSERT(M::Nr == N   &&   M::Nc == N);
            MX< SliceProxy<N1, N1, M::Rs, M::Cs, T, const T> > a( m.constBlock<N1,N1>( 0, 0 ) );
            MX< SliceProxy<N1, N2, M::Rs, M::Cs, T, const T> > b( m.constBlock<N1,N2>( 0, N1) );
            MX< SliceProxy<N2, N1, M::Rs, M::Cs, T, const T> > c( m.constBlock<N2,N1>(N1, 0 ) );
            MX< SliceProxy<N2, N2, M::Rs, M::Cs, T, const T> > d( m.constBlock<N2,N2>(N1, N1) );

            MX< Data<N, N, T> > result;
            MX< SliceProxy<N1, N1, M::Rs, M::Cs, T, T> > ar( result.block<N1,N1>( 0, 0) );
            MX< SliceProxy<N1, N2, M::Rs, M::Cs, T, T> > br( result.block<N1,N2>( 0, N1) );
            MX< SliceProxy<N2, N1, M::Rs, M::Cs, T, T> > cr( result.block<N2,N1>(N1, 0) );
            MX< SliceProxy<N2, N2, M::Rs, M::Cs, T, T> > dr( result.block<N2,N2>(N1, N1) );

            MX< Data<N1, N1, T> > ai = a.blockInv();
            MX< Data<N2, N2, T> > di = d.blockInv();

            ar = (a-b*di*c).blockInv();
            cr = -di*c*ar;
            dr = (d-c*ai*b).blockInv();
            br = -ai*b*dr;
            return result;
            }
    };

template<>
template<class D>
inline MX< Data<2, 2, typename D::value_type> >  BlockInv<2>::blockInv(const MX<D>& m) {
    typedef MX<D> M;
    typedef typename M::value_type T;
    COMPILE_ASSERT(M::Nr == 2   &&   M::Nc == 2);
    MX< Data<2, 2, T> > result;
    T a = m(0,0),   b = m(0,1),   c = m(1,0),   d = m(1,1);
    T idet = 1/(a*d - b*c);
    T *rd = result.data().data();
    rd[0] = d*idet;
    rd[1] = -b*idet;
    rd[2] = -c*idet;
    rd[3] = a*idet;
    return result;
}

template<>
template<class D>
inline MX< Data<1, 1, typename D::value_type> >  BlockInv<1>::blockInv(const MX<D>& m)
{
    typedef MX<D> M;
    typedef typename M::value_type T;
    COMPILE_ASSERT(M::Nr == 1   &&   M::Nc == 1);
    MX< Data<1, 1, T> > result;
    result(0,0) = 1/m(0,0);
    return result;
}



template< class T >
class type_util {};

template<>
class type_util<double> {
public:
    typedef double norm_type;
    static double norm(double x) {
        return fabs(x);
    }
};

template< int n, class T = double >
class GaussSolver
{
public:
    typedef T value_type;
    typedef typename type_util<T>::norm_type TN;
    typedef Data<n, n, T> D1;
    typedef MX< D1 > M1;
    typedef typename M1::Row R1;
    enum { N = n };

    static int pivot(const R1& row)
    {
        // Find pivoting column
        int icp = 0;
        T *p = row.data().data();
        TN acp = type_util<T>::norm(*p);
        p+=D1::Cs;
        for (int i=1; i<N; ++i, p+=D1::Cs) {
            TN ac = type_util<T>::norm(*p);
            if (acp < ac) {
                acp = ac;
                icp = i;
            }
        }
        return icp;
    }

    template< int n2 >
    static void gauss(M1& a, MX< Data<n,n2> >& b, int piv[N])
    {
        typedef MX< Data<n,n2> > M2;
        typedef typename M2::Row R2;
        for(int ir=0; ir+1<N; ++ir) {
            R1 ra = a.row(ir);
            R2 rb = b.row(ir);

            // Find pivoting column
            int icp = piv[ir] = pivot(ra);
            T ix = 1 / ra(0, icp);
            for (int ir2=ir+1; ir2<N; ++ir2) {
                R1 ra2 = a.row(ir2);
                R2 rb2 = b.row(ir2);
                T factor = ra2(0, icp) * ix;
                ra2 -= ra*factor;
                rb2 -= rb*factor;
            }
        }
        piv[N-1] = pivot(a.row(N-1));

        for(int ir=N-1; ir>=0; --ir) {
            R1 ra = a.row(ir);
            R2 rb = b.row(ir);
            int icp = piv[ir];
            T factor = 1 / ra(0, icp);
            ra *= factor;
            rb *= factor;
            for (int ir2=ir-1; ir2>=0; --ir2)
            {
                R1 ra2 = a.row(ir2);
                R2 rb2 = b.row(ir2);
                T factor = ra2(0, icp);
                ra2 -= ra*factor;
                rb2 -= rb*factor;
            }
        }
    }
    template< class D1, class D2 >
    static MX< Data<n, D2::Nc, T> > solve(const MX<D1>& a, const MX<D2>& b)
    {
        M1 ca = a;
        MX< Data<n, D2::Nc, T> > cb = b;
        int piv[N];
        gauss(ca, cb, piv);
        MX< Data<n, D2::Nc, T> > result;
        for (int i=0; i<N; ++i)
            result.row(piv[i]) = cb.row(i);
        return result;
    }

};

template< class D >
inline MX< Data< D::Nr, D::Nc, typename D::value_type> > MX<D>::blockInv() const
{
    return BlockInv<Nr>::blockInv(*this);
}

template< class D >
inline MX< Data< D::Nr, D::Nc, typename D::value_type> > MX<D>::inv() const
{
    MX< fsmx::Data< Nr, Nc, value_type > > f = zeros();
    f.diag().fill(1);
    return gaussSolve(f);
}

template< class D >
template< class D2 >
inline MX< fsmx::Data<D::Nr, D2::Nc, typename D::value_type> > MX<D>::gaussSolve(const MX<D2>& f) const
{
    return GaussSolver<Nr, value_type>::solve(*this, f);
}

template< class S, class D > void printMatrix(S& s, const fsmx::MX<D>& m)
{
    typedef fsmx::MX<D> M;
    for (int r=0; r<M::Nr; ++r) {
        for (int c=0; c<M::Nc; ++c) {
            s << m(r,c);
            if (c+1 == M::Nc)
                s << '\n';
            else
                s << '\t';
        }
    }
}

template< class S, class D >
S& operator<<(S& s, const fsmx::MX<D>& m) {
    printMatrix(s, m);
    return s;
}

#define CTM_FSMX_T typename D::value_type
#define CTM_FSMX_R const CTM_FSMX_T&

template< class D > inline void mxSet(const MX<D>& m, CTM_FSMX_R x) {
    COMPILE_ASSERT(D::Length == 1);
    *m.data().data() = x;
}
template< class D > inline void mxSet(MX<D>& m, CTM_FSMX_R x) {
    COMPILE_ASSERT(D::Length == 1);
    *m.data().data() = x;
}
template< class D > inline void mxSet(const MX<D>& m, CTM_FSMX_R x1, CTM_FSMX_R x2) {
    COMPILE_ASSERT(D::Length == 2);
    CTM_FSMX_T x[] = { x1, x2 };
    m.set(x);
}
template< class D > inline void mxSet(MX<D>& m, CTM_FSMX_R x1, CTM_FSMX_R x2) {
    COMPILE_ASSERT(D::Length == 2);
    CTM_FSMX_T x[] = { x1, x2 };
    m.set(x);
}
template< class D > inline void mxSet(const MX<D>& m, CTM_FSMX_R x1, CTM_FSMX_R x2, CTM_FSMX_R x3) {
    COMPILE_ASSERT(D::Length == 3);
    CTM_FSMX_T x[] = { x1, x2, x3 };
    m.set(x);
}
template< class D > inline void mxSet(MX<D>& m, CTM_FSMX_R x1, CTM_FSMX_R x2, CTM_FSMX_R x3) {
    COMPILE_ASSERT(D::Length == 3);
    CTM_FSMX_T x[] = { x1, x2, x3 };
    m.set(x);
}
template< class D > inline void mxSet(const MX<D>& m, CTM_FSMX_R x1, CTM_FSMX_R x2, CTM_FSMX_R x3, CTM_FSMX_R x4) {
    COMPILE_ASSERT(D::Length == 4);
    CTM_FSMX_T x[] = { x1, x2, x3, x4 };
    m.set(x);
}
template< class D > inline void mxSet(MX<D>& m, CTM_FSMX_R x1, CTM_FSMX_R x2, CTM_FSMX_R x3, CTM_FSMX_R x4) {
    COMPILE_ASSERT(D::Length == 4);
    CTM_FSMX_T x[] = { x1, x2, x3, x4 };
    m.set(x);
}
template< class D > inline void mxSet(const MX<D>& m, CTM_FSMX_R x1, CTM_FSMX_R x2, CTM_FSMX_R x3, CTM_FSMX_R x4, CTM_FSMX_R x5) {
    COMPILE_ASSERT(D::Length == 5);
    CTM_FSMX_T x[] = { x1, x2, x3, x4, x5 };
    m.set(x);
}
template< class D > inline void mxSet(MX<D>& m, CTM_FSMX_R x1, CTM_FSMX_R x2, CTM_FSMX_R x3, CTM_FSMX_R x4, CTM_FSMX_R x5) {
    COMPILE_ASSERT(D::Length == 5);
    CTM_FSMX_T x[] = { x1, x2, x3, x4, x5 };
    m.set(x);
}
template< class D > inline void mxSet(const MX<D>& m, CTM_FSMX_R x1, CTM_FSMX_R x2, CTM_FSMX_R x3, CTM_FSMX_R x4, CTM_FSMX_R x5, CTM_FSMX_R x6) {
    COMPILE_ASSERT(D::Length == 6);
    CTM_FSMX_T x[] = { x1, x2, x3, x4, x5, x6 };
    m.set(x);
}
template< class D > inline void mxSet(MX<D>& m, CTM_FSMX_R x1, CTM_FSMX_R x2, CTM_FSMX_R x3, CTM_FSMX_R x4, CTM_FSMX_R x5, CTM_FSMX_R x6) {
    COMPILE_ASSERT(D::Length == 6);
    CTM_FSMX_T x[] = { x1, x2, x3, x4, x5, x6 };
    m.set(x);
}
template< class D > inline void mxSet(const MX<D>& m, CTM_FSMX_R x1, CTM_FSMX_R x2, CTM_FSMX_R x3, CTM_FSMX_R x4, CTM_FSMX_R x5, CTM_FSMX_R x6, CTM_FSMX_R x7) {
    COMPILE_ASSERT(D::Length == 7);
    CTM_FSMX_T x[] = { x1, x2, x3, x4, x5, x6, x7 };
    m.set(x);
}
template< class D > inline void mxSet(MX<D>& m, CTM_FSMX_R x1, CTM_FSMX_R x2, CTM_FSMX_R x3, CTM_FSMX_R x4, CTM_FSMX_R x5, CTM_FSMX_R x6, CTM_FSMX_R x7) {
    COMPILE_ASSERT(D::Length == 7);
    CTM_FSMX_T x[] = { x1, x2, x3, x4, x5, x6, x7 };
    m.set(x);
}
template< class D > inline void mxSet(const MX<D>& m, CTM_FSMX_R x1, CTM_FSMX_R x2, CTM_FSMX_R x3, CTM_FSMX_R x4, CTM_FSMX_R x5, CTM_FSMX_R x6, CTM_FSMX_R x7, CTM_FSMX_R x8) {
    COMPILE_ASSERT(D::Length == 8);
    CTM_FSMX_T x[] = { x1, x2, x3, x4, x5, x6, x7, x8 };
    m.set(x);
}
template< class D > inline void mxSet(MX<D>& m, CTM_FSMX_R x1, CTM_FSMX_R x2, CTM_FSMX_R x3, CTM_FSMX_R x4, CTM_FSMX_R x5, CTM_FSMX_R x6, CTM_FSMX_R x7, CTM_FSMX_R x8) {
    COMPILE_ASSERT(D::Length == 8);
    CTM_FSMX_T x[] = { x1, x2, x3, x4, x5, x6, x7, x8 };
    m.set(x);
}
template< class D > inline void mxSet(const MX<D>& m, CTM_FSMX_R x1, CTM_FSMX_R x2, CTM_FSMX_R x3, CTM_FSMX_R x4, CTM_FSMX_R x5, CTM_FSMX_R x6, CTM_FSMX_R x7, CTM_FSMX_R x8, CTM_FSMX_R x9) {
    COMPILE_ASSERT(D::Length == 9);
    CTM_FSMX_T x[] = { x1, x2, x3, x4, x5, x6, x7, x8, x9 };
    m.set(x);
}
template< class D > inline void mxSet(MX<D>& m, CTM_FSMX_R x1, CTM_FSMX_R x2, CTM_FSMX_R x3, CTM_FSMX_R x4, CTM_FSMX_R x5, CTM_FSMX_R x6, CTM_FSMX_R x7, CTM_FSMX_R x8, CTM_FSMX_R x9) {
    COMPILE_ASSERT(D::Length == 9);
    CTM_FSMX_T x[] = { x1, x2, x3, x4, x5, x6, x7, x8, x9 };
    m.set(x);
}

#undef CTM_FSMX_R
#undef CTM_FSMX_T

#define CTM_FSMX_R const typename M::value_type&

template< class M > inline M mxMake(CTM_FSMX_R x) {
    M m;
    *m.data.data() = x;
    return m;
}
template< class M > inline M mxMake(CTM_FSMX_R x1, CTM_FSMX_R x2) {
    M m;
    mxSet(m, x1, x2);
    return m;
}
template< class M > inline M mxMake(CTM_FSMX_R x1, CTM_FSMX_R x2, CTM_FSMX_R x3) {
    M m;
    mxSet(m, x1, x2, x3);
    return m;
}
template< class M > inline M mxMake(CTM_FSMX_R x1, CTM_FSMX_R x2, CTM_FSMX_R x3, CTM_FSMX_R x4) {
    M m;
    mxSet(m, x1, x2, x3, x4);
    return m;
}
template< class M > inline M mxMake(CTM_FSMX_R x1, CTM_FSMX_R x2, CTM_FSMX_R x3, CTM_FSMX_R x4, CTM_FSMX_R x5) {
    M m;
    mxSet(m, x1, x2, x3, x4, x5);
    return m;
}
template< class M > inline M mxMake(CTM_FSMX_R x1, CTM_FSMX_R x2, CTM_FSMX_R x3, CTM_FSMX_R x4, CTM_FSMX_R x5, CTM_FSMX_R x6) {
    M m;
    mxSet(m, x1, x2, x3, x4, x5, x6);
    return m;
}
template< class M > inline M mxMake(CTM_FSMX_R x1, CTM_FSMX_R x2, CTM_FSMX_R x3, CTM_FSMX_R x4, CTM_FSMX_R x5, CTM_FSMX_R x6, CTM_FSMX_R x7) {
    M m;
    mxSet(m, x1, x2, x3, x4, x5, x6, x7);
    return m;
}
template< class M > inline M mxMake(CTM_FSMX_R x1, CTM_FSMX_R x2, CTM_FSMX_R x3, CTM_FSMX_R x4, CTM_FSMX_R x5, CTM_FSMX_R x6, CTM_FSMX_R x7, CTM_FSMX_R x8) {
    M m;
    mxSet(m, x1, x2, x3, x4, x5, x6, x7, x8);
    return m;
}
template< class M > inline M mxMake(CTM_FSMX_R x1, CTM_FSMX_R x2, CTM_FSMX_R x3, CTM_FSMX_R x4, CTM_FSMX_R x5, CTM_FSMX_R x6, CTM_FSMX_R x7, CTM_FSMX_R x8, CTM_FSMX_R x9) {
    M m;
    mxSet(m, x1, x2, x3, x4, x5, x6, x7, x8, x9);
    return m;
}

#undef CTM_FSMX_R

} // end namespace fsmx

#endif // FSMX_H
