//------------------------------------------------------------------------------
//   TumanakoVC - Electric Vehicle and Motor control software
//   Copyright (C) 2011 Graeme Bell <graemeb@users.sourceforge.net>
//
//   This file is part of TumanakoVC.
//
//   TumanakoVC is free software: you can redistribute it and/or modify
//   it under the terms of the GNU Lesser General Public License as published
//   by the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
//   TumanakoVC is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU Lesser General Public License for more details.
//
//   You should have received a copy of the GNU Lesser General Public License
//   along with TumanakoVC.  If not, see <http://www.gnu.org/licenses/>.
//
// DESCRIPTION:
//   This file provides a relatively basic template-based clas for performing
//   fixed-point arithmetic.
//
// HISTORY:
//   Graeme Bell 12/June/2011 - First Cut
//------------------------------------------------------------------------------

#ifndef FIXEDPOINT_H
#define FIXEDPOINT_H


/**************************************************************************************************
                          Fixed-Point Arithmetic Template Class
***************************************************************************************************

As the name implies, a fixed number of bits (or Q-bits as they are often called) is reserved at the
bottom of an integer number for representing the fractional part of an integer. For instance
reserving 4 qbits allows you to represent numbers as small as 1/(1<<4) = 1/16th. Because the number
of qbits is fixed, arithmetic on fixed-point numbers can be as efficient as normal integer arithmetic.

One of the significant problems with using fixed-point numbers is managing how many qbits a variable
has and making sure that operations on it respect the number of qbits. For instance if you want to
add 1 to a fixed-point variable with 4 qbits you must actually add 1<<4 otherwise you will be adding
incompatable values (a 4-qbit value with a 0-qbit value).

This is were a template based fixed-point class can be usefull, it can remember the number of qbits
a value has, and automatically handle (or warn you about) incompatable operations without the need
to actually store this information at run-time. Allowing the compiler to potentially produce code
as good as normal integer arithmetic.

While this implementation attempts to address the above area, it doesn't address one of the other
problems with managing fixed-point number, namely ensuring values don't overflow or underflow. So 
carefull consideration should be given to maximum range values can take, and how various operations
will effect this.


As a general philosophy this implementation requires the user to explicity handle any operations
that would require a reduction in the precision of an argument. This means that given,

    typedef tFixedPoint<8>  tQ8;
    typedef tFixedPoint<12> tQ12;
    
    tQ8   a8, b8;
    tQ12  c12;
    
operations like
    
    a8 = b8 + c12;
    
which require a Q12 value to be reduced to a lesser precision Q8 value should at least produce a 
warning by the compiler (most likely about a negative shift value).

To handle this situation several methods have been provided for reducing the precision of values.
For instance the above operation could be written as any of,

    a8 = b8 + c12.roundedTo( b8 );        // round c12 down to the same precision as b8
    a8 = b8 + c12.roundedTo< 8 >();       // round c12 down to 8 qbits
    a8 = b8 + c12.roundedTo< tQ8 >();     // round c12 down to the same precision as fixed-point type tQ8
    
where "roundedTo" could also be replaced with "truncatedTo" if preferred. The opposite case, which
requires increasing the precision of an argument, should compile without warnings, but you can also
explicitly increase a variables precision by using the "increasedTo" methods.

    c12 = a8 + b8;                     
    c12 = (a8 + b8).increasedTo( c12 );   // equivalent to the line above


Also note that the current implementation is very picky about the type of values a fixed-point 
type can be constructed from, so you may get compiler errors about ambiguous conversions. For
instance,

    typedef tFixedPoint<8,unsigned> tUQ8;
    
    tUQ8 u8 = 3;
    
will cause the compiler to complain. This can be fixed by casting the value to the correct type, or
if its a constant as in this case by making sure it has the correct type.

    tUQ8 u8 = 3u;
    
***************************************************************************************************

TODO:
  - full run-time test-bench to make sure the implementation actually works as required.
  - ensure compiled code is as good as an explicit manual implementation, especially WRT parameter
    passing.
  - better documentation.
  - improved compile-time checking (possibly including the automatic handling of some loss of 
    precision cases).
  - a debug version that provides runtime checks for over/underflows.
  - possibly a floating-point based version for validating and checking the accuracy of their usage
    in an application.
    
***************************************************************************************************/

        
//-------------------------------------------------------------------------------------------------
// Support Macros

/** This macro is used to enable all methods and functions that provide support for the standard
    floating-point double type **/
#define FIXEDPOINT_ENABLE_FLOATINGPOINT_SUPPORT        


/** These macros are designed to allow you to specify constants in a similar way to how you would
    a true floating-point constant. eg. -1.25 can be written as "FIXEDPOINT_CONSTANT( tQ4, -1,25 )"
    and will be automatically converted to a fixed-point value of the specified type. This allows
    you to define fixed-point constants in a more natural way when float-point support has not been
    endabled.
        For frequent use it would pay to define your own more concise version of the macro, for 
    instance "#define Q4CONST( dec,frac )  FIXEDPOINT_CONSTANT( tQ4, dec,frac )" **/
#define FIXEDPOINT_CONSTANT( FPType, dec,frac )    \
    FPType( dec, (((1 << FPType::cQBits)*frac + (2##frac-1##frac)/2)/(2##frac-1##frac)), FPType::cQBits )

/** This macro is provided for convenience to simplify the cases where you don't have a suitable
    typedef already defined for use in the previous macro. It assumes you are using the default
    tFixedPoint data-type **/
#define FIXEDPOINT_CONSTANTQ( qbits, dec,frac )  FIXEDPOINT_CONSTANT( tFixedPoint<qbits>, dec,frac )


/** This macro lets you control the implementation used for division. Currently it rounds towards
    +ve infinity, but you may prefer to just truncate or improve it to round away from 0 instead **/
#define FIXEDPOINT_IMPL_DIVIDE( dividend, divisor )  (((divisor) + ((dividend) >> 1)) / (dividend))

/** This macro lets you control the implementation used for rounding. Currently it rounds towards
    +ve infinity, but you may prefer something else such as to round away from 0 instead. It should
    not be changed to just truncate because separate methods are provided for this.
        A macro was used to implement this to make it more likely the compiler would produce an
    error or warning when 'byQBits' is incorrectly negative. Also note that this macro does not
    cover the rounding used for floating-point types **/
#define FIXEDPOINT_IMPL_ROUND( value, byQBits )  (((value) + (1 << ((byQBits)-1))) >> (byQBits))


//-------------------------------------------------------------------------------------------------
// Class Definition
    
/** Template class for fixed-point artithmetic. Where 'QBits' is the number of bits reserved to
    hold the fractional part of the vlaue **/
template< int QBits, typename DataType = int >
class tFixedPoint
{
public:
    /** Records the number of qbits used to hold the fractional part of this fixed point class **/
    static const unsigned cQBits = QBits;
    
    /** Records the underlying type used to store fixed-point values (the combined interger and 
        fractional parts) **/
    typedef DataType tValue;

    //---------------------------------------------------------------------------------------------
    // Construction

    /** Creates an instance of a fixed-point with an initial value that already contains the 
        required number of qbits. eg. if you want a value of 1.25 for a tFixedPoint<4> variable
        you would provide a 'qValue' of 20. 
            This is mainly provided for internal use by this class, but may be useful for 
        implementing extended fixed-point functionality externally **/
    static tFixedPoint create( tValue qValue ) { return tFixedPoint( qValue, QBits ); }
    
    tFixedPoint() : value_() {}
    tFixedPoint( const tFixedPoint& value ) : value_( value.value_ ) {}
    
    /** Constructs a fixed-point value from a variable or constant of the same underlying tValue
        type. The 'value' provided is assumed to have no/0 qbits and will be adjusted appropriately **/
    tFixedPoint( tValue value ) : value_( value << QBits ) {}

    /** Constructs a fixed-point value from a variable or constant that already has some 'qBits'
        incorporated in it. The 'qValue' must be of equal or lower precision than this class (ie.
        'qBits' <= cQBits) or an invalid value may be constructed - normally resulting in a
        compilier warning or error, probably about a negative shift count **/
    tFixedPoint( tValue qValue, unsigned qBits ) : value_( qValue << (QBits - int(qBits)) ) {}
    
    /** Constructs a fixed-point value from separate integer 'intPart' and fractional 'absFracPart'
        values. This is mainly intended for use by the FIXEDPOINT_CONSTANT macros, so the
        'absFracPart' must be unsigned and the sign of the 'intPart' will used to determine what
        its sign should be. eg. "tFixedPoint<4>( -1, 4, 4 ) would produce a value equivalent to
        -1.25 **/
    tFixedPoint( tValue intPart, tValue absFracPart, unsigned qFracBits )
        : value_( (intPart >= 0)? ((intPart << QBits)+(absFracPart << (QBits - int(qFracBits)))) 
                                : ((intPart << QBits)-(absFracPart << (QBits - int(qFracBits)))) ) {}
    
    /** Constructs a fixed-point value from another fixed-point value of a different type. The
        source type must be of lower precision (value.cQBits <= cQBits) than this type or the
        constructed value will be incorrect.
            This situation should result in a compilier warning or error, probably about a negative
        shift count. Use one of the "roundedTo" or "truncatedTo" methods on the source fixed-point
        value to make sure it has lower precision. eg. "tFixedPoint<4> x4( x8.roundedTo<4>() )" **/
    template< int QBits2, typename DataType2 > tFixedPoint( const tFixedPoint<QBits2,DataType2>& value )
        : value_( value.qValue() << (QBits - QBits2) ) {}

    //---------------------------------------------------------------------------------------------
    // Conversions

    /** Returns the underlying value, which includes the extra qbits. eg. if the current value of a
        tFixedPoint<4> variable is equivalent to 1.25 it will return the value 20 **/
    tValue qValue() const { return value_; }
    
    /** These methods convert a fixed-point value to the same type as another fixed-point variable.
        Especially usefull when you need to reduce the precision of a value to match another lower
        precision one, eg. given "tQ4 x4; tQ8 y8;" then "x4 = y8;" won't work but "x4 = y8.roundedTo( x4 );"
        will. And will continue to work well if x4 becomes an x3, where as "x4 = y8.roundedTo<4>( y8 )"
        will not.
            Two of the methods, "truncatedTo" and "roundedTo", convert from a higher precision
        fixed-point value to a lower precision one, either truncating or rounding the original
        value respectively. While "increasedTo" convert from a lower precision value to a higher
        one - this method should not be required as often as the first two as most operations will
        automatically increase the precision to match the left hand argument if necessary **/
    template< int QBits2, typename DataType2 > tFixedPoint<QBits2,DataType2> truncatedTo( const tFixedPoint<QBits2,DataType2>& ) const
        { return tFixedPoint<QBits2,DataType2>::create( value_ >> (QBits - QBits2) ); }
    template< int QBits2, typename DataType2 > tFixedPoint<QBits2,DataType2> roundedTo(   const tFixedPoint<QBits2,DataType2>& ) const
        { return tFixedPoint<QBits2,DataType2>::create( FIXEDPOINT_IMPL_ROUND( value_, (QBits - QBits2) ) ); }
    template< int QBits2, typename DataType2 > tFixedPoint<QBits2,DataType2> increasedTo( const tFixedPoint<QBits2,DataType2>& ) const
        { return tFixedPoint<QBits2,DataType2>::create( DataType2(value_) << (QBits2 - QBits) ); }
    
    /** A variation of the above "increasedTo" method, this conversion allows you to increase the
        precision of a variable by the precision (number of qbits) of another type. Its main use
        is for division where the precision of the result is that of the dividend minus the divisor.
        eg. "x8 / y6" will result in only a tFixedPoint<2> which looses precision, where as 
        "x8.increasedBy( y6 ) / y6" ensures that the final result will be a tFixedPoint<8> **/
    template< int QBits2, typename DataType2 > tFixedPoint<QBits+QBits2,DataType2> increasedBy( const tFixedPoint<QBits2,DataType2>& ) const
        { return tFixedPoint<QBits+QBits2,DataType2>::create( DataType2(value_) << QBits2 ); }
    
    /** These methods convert a fixed-point value to the number of qbits specified, for example
        "tQ8 x8 = y12.roundedTo<8>()". 
            The first two methods convert from a higher precision fixed-point value to a lower 
        precision one, either truncating or rounding the original value respectively. While the
        last converts from a lower precision value to a higher one - this method shouldn't be
        required as often as most operations automatically increase the precision to match the
        left hand argument if necessary **/
    template< int QBits2 > tFixedPoint<QBits2,DataType> truncatedTo() const
        { return tFixedPoint<QBits2,DataType>::create( value_ >> (QBits - QBits2) ); }
    template< int QBits2 > tFixedPoint<QBits2,DataType> roundedTo()   const
        { return tFixedPoint<QBits2,DataType>::create( FIXEDPOINT_IMPL_ROUND( value_, (QBits - QBits2) ) ); }
    template< int QBits2 > tFixedPoint<QBits2,DataType> increasedTo() const
        { return tFixedPoint<QBits2,DataType>::create( value_ << (QBits2 - QBits) ); }
    
    /** These methods convert a fixed-point value to another fixed-point value with the number of
        qbits, and unlying data-type specified, for example "tQ8 x8 = y12.roundedTo<8,int>()" **/
    template< int QBits2, typename DataType2 > tFixedPoint<QBits2,DataType2> truncatedTo() const
        { return tFixedPoint<QBits2,DataType2>::create( value_ >> (QBits - QBits2) ); }
    template< int QBits2, typename DataType2 > tFixedPoint<QBits2,DataType2> roundedTo()   const
        { return tFixedPoint<QBits2,DataType2>::create( FIXEDPOINT_IMPL_ROUND( value_, (QBits - QBits2) ) ); }
    template< int QBits2, typename DataType2 > tFixedPoint<QBits2,DataType2> increasedTo() const
        { return tFixedPoint<QBits2,DataType2>::create( DataType2(value_) << (QBits2 - QBits) ); }

    /** These methods convert from one fixed-point value type to another, in a single direction
        (ie. to a lower precision using "truncateTo" or "roundTo", or a higher precision using
        "increasedTo"). eg. "tQ8 x8 = y12.roundedTo<tQ8>()" **/
    template< typename FPType > FPType truncatedTo() const { return FPType::create( value_ >> (QBits - FPType::cQBits) ); }
    template< typename FPType > FPType roundedTo()   const { return FPType::create( FIXEDPOINT_IMPL_ROUND( value_, (QBits - FPType::cQBits) ) ); }
    template< typename FPType > FPType increasedTo() const { return FPType::create( FPType::tValue(value_) << (FPType::cQBits - QBits) ); }

    //---------------------------------------------------------------------------------------------
    // Assignment
    
    /** Assignment between fixed-point types with the same precision and underlying data-type **/
    tFixedPoint& operator=( const tFixedPoint& value ) { value_ = value.value_; return *this; }
    
    /** This method allows a variable with a different fixed-point type to be assigned. The type
        being assigned must be of lower or equal precision. Use the "setTruncated" or "setRounded"
        methods to assign a higher precision type (you can also use the source fixed-point values
        "truncatedTo" or "roundedTo" methods) **/
    template< int QBits2, typename DataType2 > tFixedPoint& operator=( const tFixedPoint<QBits2,DataType2>& value )
        { return *this = value.increasedTo<QBits,DataType>(); }
    
    /** These methods provide an alternative way to assign a new fixed-point value. The "set"
        methods will automatically increase the precision of their argument if necessary. While
        the "setTruncated" and "setRounded" can be used to reduce the precision of their argument
        down to the correct value **/
    void set( const tFixedPoint& value ) { value_ = value.value_; }
    template< int QBits2, typename DataType2 > void set( const tFixedPoint< QBits2, DataType2 >& value )
        { *this = value.increasedTo< QBits >(); }
    template< int QBits2, typename DataType2 > void setTruncated( const tFixedPoint< QBits2, DataType2 >& value )
        { *this = value.truncatedTo< QBits >(); }
    template< int QBits2, typename DataType2 > void setRounded( const tFixedPoint< QBits2, DataType2 >& value )
        { *this = value.roundedTo< QBits >(); }

    //---------------------------------------------------------------------------------------------
    // Comparisons
    
    /** Returns true if the fixed-point value is zero **/
    bool operator!() const { return (value_ == 0); }
    
    bool operator==( const tFixedPoint& value ) const { return (value_ == value.value_); }
    bool operator!=( const tFixedPoint& value ) const { return (value_ != value.value_); }
    bool operator< ( const tFixedPoint& value ) const { return (value_ <  value.value_); }
    bool operator<=( const tFixedPoint& value ) const { return (value_ <= value.value_); }
    bool operator>=( const tFixedPoint& value ) const { return (value_ >= value.value_); }
    bool operator> ( const tFixedPoint& value ) const { return (value_ >  value.value_); }

    template< int QBits2, typename DataType2 > bool operator==( const tFixedPoint<QBits2,DataType2>& value ) const
        { return (value_ == value.increasedTo<QBits,DataType>()); }
    template< int QBits2, typename DataType2 > bool operator!=( const tFixedPoint<QBits2,DataType2>& value ) const
        { return (value_ != value.increasedTo<QBits,DataType>()); }
    template< int QBits2, typename DataType2 > bool operator< ( const tFixedPoint<QBits2,DataType2>& value ) const
        { return (value_ <  value.increasedTo<QBits,DataType>()); }
    template< int QBits2, typename DataType2 > bool operator<=( const tFixedPoint<QBits2,DataType2>& value ) const
        { return (value_ <= value.increasedTo<QBits,DataType>()); }
    template< int QBits2, typename DataType2 > bool operator>=( const tFixedPoint<QBits2,DataType2>& value ) const
        { return (value_ >= value.increasedTo<QBits,DataType>()); }
    template< int QBits2, typename DataType2 > bool operator> ( const tFixedPoint<QBits2,DataType2>& value ) const
        { return (value_ >  value.increasedTo<QBits,DataType>()); }
    
    //---------------------------------------------------------------------------------------------
    // Arithmetic
    
    tFixedPoint& operator+=( const tFixedPoint& value ) { value_ += value.value_; return *this; }
    tFixedPoint& operator-=( const tFixedPoint& value ) { value_ -= value.value_; return *this; }
    tFixedPoint& operator*=( const tFixedPoint& value ) { value_ = roundedBy_( value_ * value.value_, QBits ); return *this; }
    tFixedPoint& operator/=( const tFixedPoint& value ) { value_ = FIXEDPOINT_IMPL_DIVIDE( value_, value.value_ ); return *this; }
    
    /** These methods provide direct support for multipling or dividing by a constant. This is
        important for these operations as they affect the number of qbits in the result. Without
        direct support the constant would be converted into a fixed-point number unnecesserily
        increasing the possibility of an overflow occuring during the operation **/
    template< typename DataType2 > tFixedPoint& operator*=( const DataType2& value )
        { value_ *= value; return *this; }
    template< typename DataType2 > tFixedPoint& operator/=( const DataType2& value )
        { value_ = FIXEDPOINT_IMPL_DIVIDE( value_, value ); return *this; }
    
    template< int QBits2, typename DataType2 > tFixedPoint& operator+=( const tFixedPoint<QBits2,DataType2>& value )
        { value_ += value.qValue() << (QBits-QBits2); return *this; }
    template< int QBits2, typename DataType2 > tFixedPoint& operator-=( const tFixedPoint<QBits2,DataType2>& value )
        { value_ -= value.qValue() << (QBits-QBits2); return *this; }
    template< int QBits2, typename DataType2 > tFixedPoint& operator*=( const tFixedPoint<QBits2,DataType2>& value )
        { value_ = FIXEDPOINT_IMPL_ROUND( value_*value.qValue(), QBits2 ); return *this; }
    template< int QBits2, typename DataType2 > tFixedPoint& operator/=( const tFixedPoint<QBits2,DataType2>& value )
        { value_ = FIXEDPOINT_IMPL_DIVIDE( (value_ << (QBits-QBits2)), value.qValue() ); return *this; }

    
    tFixedPoint operator-() const { return create( -value_ ); }
    tFixedPoint operator+( const tFixedPoint& value ) const { return create( value_ + value.value_ ); }
    tFixedPoint operator-( const tFixedPoint& value ) const { return create( value_ - value.value_ ); }
    tFixedPoint<QBits+QBits,DataType> operator*( const tFixedPoint& value ) const
        { return tFixedPoint<QBits+QBits,DataType>::create( value_*value.value_ ); }
    tValue operator/( const tFixedPoint& value ) const
        { return FIXEDPOINT_IMPL_DIVIDE( value_, value.value_ ); }

    /** These methods provide direct support for multipling or dividing by a constant. This is
        important for these operations as they affect the number of qbits in the result. Without
        direct support the constant would be converted into a fixed-point number unnecesserily
        increasing the possibility of an overflow occuring during the operation **/
    template< typename DataType2 > tFixedPoint operator*( const DataType2& value ) const
        { return create( value_ * value ); }
    template< typename DataType2 > tFixedPoint operator/( const DataType2& value ) const
        { return create( FIXEDPOINT_IMPL_DIVIDE( value_, value ) ); }

    template< int QBits2, typename DataType2 > tFixedPoint operator+( const tFixedPoint<QBits2,DataType2>& value ) const
        { return create( value_ + (value.qValue() << (QBits-QBits2)) ); }
    template< int QBits2, typename DataType2 > tFixedPoint operator-( const tFixedPoint<QBits2,DataType2>& value ) const
        { return create( value_ - (value.qValue() << (QBits-QBits2)) ); }
    template< int QBits2, typename DataType2 > tFixedPoint<QBits+QBits2,DataType> operator*( const tFixedPoint<QBits2,DataType2>& value ) const
        { return tFixedPoint<QBits+QBits2,DataType>::create( value_ * value.qValue() ); }
    template< int QBits2, typename DataType2 > tFixedPoint<QBits-QBits2,DataType> operator/( const tFixedPoint<QBits2,DataType2>& value ) const
        { return tFixedPoint<QBits-QBits2,DataType>::create( FIXEDPOINT_IMPL_DIVIDE( value_, value.qValue() ) ); }
    
    //---------------------------------------------------------------------------------------------
    // Miscellaneous
    
    tValue absolute() const { return (value_ >= 0)? value_ : -value_; }
    tValue intPart() const { return value_ >> QBits; }
    tValue fracPart() const { return (value_ >= 0)? (value_ & ((1 << QBits) - 1)) : -((-value_) & ((1 << QBits) - 1)); }
    tValue absFracPart() const { return ((value_ >= 0)? value_ : -value_) & ((1 << QBits) - 1); }
    
    /** This method is provided as a starter for making the output/printing of fixed-point values
        easier when support for conversion to floating-point hasn't been enabled. It returns the 
        fractional part of the fixed-point value in base-10 to the number of decimal places you
        specify. eg. "printf( "%d.%03d", a8.intPart(), a8.fracPlaces(3) );".
            NB. this method should probably be shifted out into a common/shared implementation **/
    tValue fracPlaces( unsigned decimals ) { 
        tValue frac = absFracPart();
        const unsigned cXBits = 4;
        unsigned dec_qx = (1 << (cXBits-1));
        while (decimals-- > 0) dec_qx *= 10; 
        
        tValue result = 0;
        tValue mask = 1 << (QBits - 1);
        
        while (frac != 0) {
            if (frac & mask) {
                frac -= mask;
                result += dec_qx;
            }
            dec_qx >>= 1;
            mask >>= 1;
        }
        return ((result + (1 << (cXBits-1))) >> cXBits);
    }
    
    //---------------------------------------------------------------------------------------------
    // Floating Point
    
#   ifdef FIXEDPOINT_ENABLE_FLOATINGPOINT_SUPPORT
public:    
    explicit tFixedPoint( double value ) : value_( rounded_( value ) ) {}
   
    double toDouble() const { return double( value_ ) / (tValue(1) << QBits); }

    tFixedPoint& operator=( double value ) { value_ = rounded_( value ); return *this; }

    void setTruncated( double value ) { value_ = truncated_( value ); }
    void setRounded( double value ) { value_ = rounded_( value ); }
    
    bool operator==( double value ) const { return (value_ == rounded_( value )); }
    bool operator!=( double value ) const { return (value_ != rounded_( value )); }
    bool operator< ( double value ) const { return (value_ <  rounded_( value )); }
    bool operator<=( double value ) const { return (value_ <= rounded_( value )); }
    bool operator>=( double value ) const { return (value_ >= rounded_( value )); }
    bool operator> ( double value ) const { return (value_ >  rounded_( value )); }

    tFixedPoint& operator+=( double value ) { value_ += rounded_( value ); return *this; }
    tFixedPoint& operator-=( double value ) { value_ -= rounded_( value ); return *this; }
    tFixedPoint& operator*=( double value ) { value_ = tValue( value_*value + 0.5 ); return *this; }
    tFixedPoint& operator/=( double value ) { value_ = tValue( value_/value + 0.5 ); return *this; }

    tFixedPoint operator+( double value ) const { return create( value_ + rounded_( value ) ); }
    tFixedPoint operator-( double value ) const { return create( value_ - rounded_( value ) ); }
    tFixedPoint operator*( double value ) const { return create( tValue( value_*value + 0.5 ) ); }
    tFixedPoint operator/( double value ) const { return create( tValue( value_/value + 0.5 ) ); }

    static tFixedPoint truncated( double value ) { return create( truncated_( value ) ); }
    static tFixedPoint rounded( double value ) { return create( rounded_( value ) ); }
    
private:    
    static tValue truncated_( double value ) { return tValue( value * (tValue(1) << QBits) ); }
    static tValue rounded_( double value ) { return tValue( value * (tValue(1) << QBits) + 0.5 ); }
#   endif
    
    //---------------------------------------------------------------------------------------------
    // Implementation Details
    
private:
    tValue  value_;
};

//-------------------------------------------------------------------------------------------------
// External Helpers

template< int QBits, typename DataType > tFixedPoint<QBits,DataType> operator+( DataType lhs, const tFixedPoint<QBits,DataType>& rhs ) { return rhs + lhs; }
template< int QBits, typename DataType > tFixedPoint<QBits,DataType> operator-( DataType lhs, const tFixedPoint<QBits,DataType>& rhs ) { return tFixedPoint<QBits,DataType>(lhs) - rhs; }
template< int QBits, typename DataType > tFixedPoint<QBits,DataType> operator*( DataType lhs, const tFixedPoint<QBits,DataType>& rhs ) { return rhs * lhs; }

template< int QBits, typename DataType > bool operator==( DataType lhs, const tFixedPoint<QBits,DataType>& rhs ) { return (rhs == lhs); }
template< int QBits, typename DataType > bool operator!=( DataType lhs, const tFixedPoint<QBits,DataType>& rhs ) { return (rhs != lhs); }
template< int QBits, typename DataType > bool operator< ( DataType lhs, const tFixedPoint<QBits,DataType>& rhs ) { return (rhs >= lhs); }
template< int QBits, typename DataType > bool operator<=( DataType lhs, const tFixedPoint<QBits,DataType>& rhs ) { return (rhs >  lhs); }
template< int QBits, typename DataType > bool operator>=( DataType lhs, const tFixedPoint<QBits,DataType>& rhs ) { return (rhs <  lhs); }
template< int QBits, typename DataType > bool operator> ( DataType lhs, const tFixedPoint<QBits,DataType>& rhs ) { return (rhs <= lhs); }

#ifdef FIXEDPOINT_ENABLE_FLOATINGPOINT_SUPPORT
template< int QBits > tFixedPoint<QBits> truncatedTo( double value ) { return tFixedPoint<QBits>::truncated( value ); }
template< int QBits > tFixedPoint<QBits> roundedTo(   double value ) { return tFixedPoint<QBits>::rounded( value ); }

template< typename FPType > FPType truncatedTo( double value ) { return FPType::truncated( value ); }
template< typename FPType > FPType roundedTo(   double value ) { return FPType::rounded( value ); }

template< int QBits, typename DataType > tFixedPoint<QBits,DataType> truncatedTo( const tFixedPoint<QBits,DataType>&, double value )
    { return tFixedPoint<QBits,DataType>::truncated( value ); }
template< int QBits, typename DataType > tFixedPoint<QBits,DataType> roundedTo(   const tFixedPoint<QBits,DataType>&, double value )
    { return tFixedPoint<QBits,DataType>::rounded( value ); }
#endif
    
//-------------------------------------------------------------------------------------------------

#endif  // inclusion guard
