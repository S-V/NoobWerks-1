/*
=============================================================================
=============================================================================
*/
#include <Base/Base.h>
#include <Core/Util/ScopedTimer.h>
#include <Core/Serialization/Text/TxTCommon.h>
#include <Core/Serialization/Text/TxTWriter.h>

mxSTOLEN("Jansson");
/* Work around nonstandard isnan() and isinf() implementations */
#ifndef isnan
static inline int isnan(double x) { return x != x; }
#endif
#ifndef isinf
static inline int isinf(double x) { return !isnan(x) && isnan(x - x); }
#endif

namespace SON
{
	static bool bDebug = false;

	struct DumpContext {
		U32	depth;
	};
	static void Print_Indent( const U32 _depth, TextStream &_stream )
	{
		_stream << '\n';
		for( UINT i = 0; i < _depth; i++ ) {
			_stream << '\t';
		}
	}
	static void Print_NameComment( const Node* _node, TextStream &_stream )
	{
		if( _node->name && _node->value.l.size > 0 )
		{
			_stream.PrintF( ";%s", _node->name );
		}
	}
	ERet DumpValueToStream( const Node* _node, const U32 _depth, TextStream &_stream, int _index = -1 )
	{
		String32 valueIndex;
		if(_index != -1) {
			Str::Format( valueIndex, "[%d] ", _index);
		}

		switch( _node->tag.type )
		{
		case TypeTag_Nil :
			_stream << Chars("nil");
			break;

		case TypeTag_List :
			{
				const U32 count = _node->value.l.size;
				if( count )
				{
					_stream << Chars("(");

					if( !count )
					{
						_stream << Chars(")");
					}
					else
					{
						if( bDebug ) {
							_stream.PrintF(" ; %s%u items", valueIndex.SafeGetPtr(), count);
						}

						const U32 listDepth = _depth + 1;

						const Node* child = _node->value.l.kids;
						int i = 0;
						while( child )
						{
							Print_Indent(listDepth, _stream);
							DumpValueToStream(child, listDepth, _stream, i);
							child = child->next;
							i++;
						}

						Print_Indent(_depth, _stream);
						_stream << Chars(")");
						if( bDebug ) {
							Print_NameComment(_node, _stream);
						}
					}
				}
				else
				{
					_stream << Chars("()");
				}
			}
			break;

		case TypeTag_Array :
			{
				const U32 count = _node->value.a.size;
				if( count )
				{
					_stream << Chars("[");

					if( bDebug ) {
						_stream.PrintF(" ; %s%u items", valueIndex.SafeGetPtr(), count);
					}

					const U32 arrayDepth = _depth + 1;

					for( UINT i = 0; i < count; i++ )
					{
						const Node* child = _node->value.a.kids + i;
						Print_Indent(arrayDepth, _stream);
						DumpValueToStream(child, arrayDepth, _stream, i);
					}

					Print_Indent(_depth, _stream);

					_stream << Chars("]");

					if( bDebug ) {
						Print_NameComment(_node, _stream);
					}					
				}
				else
				{
					_stream << Chars("[]");
				}
			}
			break;

		case TypeTag_Object :
			{
				const U32 count = _node->value.o.size;
				if( count )
				{
					_stream << Chars("{");

					if( bDebug ) {
						_stream.PrintF(" ; %s%u values", valueIndex.SafeGetPtr(), count);
					}

					const U32 objectDepth = _depth + 1;

					const Node* child = _node->value.o.kids;
					int i = 0;
					while( child )
					{
						Print_Indent(objectDepth, _stream);
						_stream.PrintF("%s = ", child->name);
						DumpValueToStream(child, objectDepth, _stream, i);
						child = child->next;
						i++;
					}
					Print_Indent(_depth, _stream);
					_stream << Chars("}");
					if( bDebug ) {
						Print_NameComment(_node, _stream);
					}
				}
				else
				{
					_stream << Chars("{}");
				}
			}
			break;

		case TypeTag_String :
			{
				//NOTE: this cannot be used with some strings, e.g. 0xCAFEBABE
				//_stream.PrintF("\'%s\'", AsString(_node));
				_stream.VWrite( "\'", 1 );
				const StringValue& s = _node->value.s;
				if( s.length > 0 ) {
					_stream.VWrite( s.start, s.length );
				}
				_stream.VWrite( "\'", 1 );
			}
			break;

		case TypeTag_Number :
			{
				const double value = AsDouble(_node);
				mxASSERT2(!isnan(value) && !isinf(value), "Invalid floating-point value!");
				double integralPart;
				double fractionalPart = modf(value, &integralPart);
				if( fabs(fractionalPart) < FLOAT64_EPSILON ) {
					_stream.PrintF("%I64d", (INT64)integralPart );
				} else {
					_stream.PrintF("%.17g", value);
				}
			}
			break;

			mxNO_SWITCH_DEFAULT;
		}
		return ALL_OK;
	}

	ERet WriteToStream( const Node* _root, AWriter &write_stream )
	{
		//ScopedTimer	timer( "SON::WriteToStream" );
		TextStream	writer(write_stream);

		if( _root->tag.type == TypeTag_Object )
		{
			const Node* child = _root->value.o.kids;
			while( child )
			{
				writer.PrintF("%s = ", child->name);
				DumpValueToStream(child, 0, writer);
				writer << '\n';
				child = child->next;
			}
		}
		else
		{
			mxDO(DumpValueToStream(_root, 0, writer));
		}

		return ALL_OK;
	}

}//namespace SON

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
