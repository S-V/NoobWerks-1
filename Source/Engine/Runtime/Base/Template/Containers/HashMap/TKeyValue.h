/*
=============================================================================
	File:	TKeyValue.h
	Desc:	
=============================================================================
*/
#pragma once

/*
====================================================================
	
	Key/Value pair objects are used by most associative container classes,
	like Dictionary or HashTable. 

====================================================================
*/
mxSTOLEN("Nebula 3");
template< typename KEYTYPE, typename VALUETYPE >
class TKeyValue {
public:
	KEYTYPE key;
	VALUETYPE value;

public:
    mxFORCEINLINE TKeyValue();
    mxFORCEINLINE TKeyValue(const KEYTYPE& k, const VALUETYPE& v);
    mxFORCEINLINE TKeyValue(const KEYTYPE& k);
    mxFORCEINLINE TKeyValue(const TKeyValue<KEYTYPE, VALUETYPE>& rhs);

    mxFORCEINLINE void operator=(const TKeyValue<KEYTYPE, VALUETYPE>& rhs);
    mxFORCEINLINE bool operator==(const TKeyValue<KEYTYPE, VALUETYPE>& rhs) const;
    mxFORCEINLINE bool operator!=(const TKeyValue<KEYTYPE, VALUETYPE>& rhs) const;
    mxFORCEINLINE bool operator>(const TKeyValue<KEYTYPE, VALUETYPE>& rhs) const;
    mxFORCEINLINE bool operator>=(const TKeyValue<KEYTYPE, VALUETYPE>& rhs) const;
    mxFORCEINLINE bool operator<(const TKeyValue<KEYTYPE, VALUETYPE>& rhs) const;
    mxFORCEINLINE bool operator<=(const TKeyValue<KEYTYPE, VALUETYPE>& rhs) const;
    mxFORCEINLINE VALUETYPE& Value();
    mxFORCEINLINE const KEYTYPE& Key() const;
    mxFORCEINLINE const VALUETYPE& Value() const;

	mxFORCEINLINE friend AWriter& operator << ( AWriter& file, const TKeyValue& o )
	{
		file << o.key << o.value;
		return file;
	}
	mxFORCEINLINE friend AReader& operator >> ( AReader& file, TKeyValue& o )
	{
		file >> o.key >> o.value;
		return file;
	}
	mxFORCEINLINE friend mxArchive& operator && ( mxArchive& archive, TKeyValue& o )
	{
		return archive && o.key && o.value;
	}
};

//--------------------------------------------------------------------------
template<class KEYTYPE, class VALUETYPE>
TKeyValue<KEYTYPE, VALUETYPE>::TKeyValue()
{
    // empty
}

//--------------------------------------------------------------------------
template<class KEYTYPE, class VALUETYPE>
TKeyValue<KEYTYPE, VALUETYPE>::TKeyValue(const KEYTYPE& k, const VALUETYPE& v) :
    key(k),
    value(v)
{
    // empty
}
//--------------------------------------------------------------------------
/**
    This strange constructor is useful for search-by-key if
    the key-value-pairs are stored in a Util::Array.
*/
template<class KEYTYPE, class VALUETYPE>
TKeyValue<KEYTYPE, VALUETYPE>::TKeyValue(const KEYTYPE& k) :
    key(k)
{
    // empty
}
//--------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
TKeyValue<KEYTYPE, VALUETYPE>::TKeyValue(const TKeyValue<KEYTYPE, VALUETYPE>& rhs) :
    key(rhs.key),
    value(rhs.value)
{
    // empty
}
//--------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
void
TKeyValue<KEYTYPE, VALUETYPE>::operator=(const TKeyValue<KEYTYPE, VALUETYPE>& rhs)
{
    this->key = rhs.key;
    this->value = rhs.value;
}

//--------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
bool
TKeyValue<KEYTYPE, VALUETYPE>::operator==(const TKeyValue<KEYTYPE, VALUETYPE>& rhs) const
{
    return (this->key == rhs.key);
}

//--------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
bool
TKeyValue<KEYTYPE, VALUETYPE>::operator!=(const TKeyValue<KEYTYPE, VALUETYPE>& rhs) const
{
    return (this->key != rhs.key);
}

//--------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
bool
TKeyValue<KEYTYPE, VALUETYPE>::operator>(const TKeyValue<KEYTYPE, VALUETYPE>& rhs) const
{
    return (this->key > rhs.key);
}

//--------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
bool
TKeyValue<KEYTYPE, VALUETYPE>::operator>=(const TKeyValue<KEYTYPE, VALUETYPE>& rhs) const
{
    return (this->key >= rhs.key);
}

//--------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
bool
TKeyValue<KEYTYPE, VALUETYPE>::operator<(const TKeyValue<KEYTYPE, VALUETYPE>& rhs) const
{
    return (this->key < rhs.key);
}

//--------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
bool
TKeyValue<KEYTYPE, VALUETYPE>::operator<=(const TKeyValue<KEYTYPE, VALUETYPE>& rhs) const
{
    return (this->key <= rhs.key);
}

//--------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
VALUETYPE&
TKeyValue<KEYTYPE, VALUETYPE>::Value()
{
    return this->value;
}

//--------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
const KEYTYPE&
TKeyValue<KEYTYPE, VALUETYPE>::Key() const
{
    return this->key;
}

//--------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
const VALUETYPE&
TKeyValue<KEYTYPE, VALUETYPE>::Value() const
{
    return this->value;
}

//----------------------------------------------------------//
//				End Of File.									//
//----------------------------------------------------------//
