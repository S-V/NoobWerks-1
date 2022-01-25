mxFORCEINLINE
bool TbMetaClass::operator == ( const TbMetaClass& other ) const
{
	return ( this == &other );
}

mxFORCEINLINE
bool TbMetaClass::operator != ( const TbMetaClass& other ) const
{
	return ( this != &other );
}

mxFORCEINLINE
const char* TbMetaClass::GetTypeName() const
{
	return m_name;
}

mxFORCEINLINE
const TypeGUID TbMetaClass::GetTypeGUID() const
{
	return m_uid;
}

mxFORCEINLINE
const TbMetaClass * TbMetaClass::GetParent() const
{
	return m_base;
}

mxFORCEINLINE
size_t TbMetaClass::GetInstanceSize() const
{
	return m_size;
}

mxFORCEINLINE
bool TbMetaClass::IsAbstract() const
{
	return m_constructor == nil;
}

mxFORCEINLINE
bool TbMetaClass::IsConcrete() const
{
	return m_constructor != nil;
}

mxFORCEINLINE
F_CreateObject * TbMetaClass::GetCreator() const
{
	return m_creator;
}

mxFORCEINLINE
F_ConstructObject *	TbMetaClass::GetConstructor() const
{
	return m_constructor;
}

mxFORCEINLINE
F_DestructObject * TbMetaClass::GetDestructor() const
{
	return m_destructor;
}

mxFORCEINLINE
const TbClassLayout& TbMetaClass::GetLayout() const
{
	return m_members;
}

//----------------------------------------------------------//
//				End Of File.									//
//----------------------------------------------------------//
