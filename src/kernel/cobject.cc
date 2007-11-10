/*                                                                              
 * PDFedit - free program for PDF document manipulation.                        
 * Copyright (C) 2006, 2007  PDFedit team:      Michal Hocko, 
 *                                              Miroslav Jahoda,       
 *                                              Jozef Misutka, 
 *                                              Martin Petricek                                             
 *
 * Project is hosted on http://sourceforge.net/projects/pdfedit                                                                      
 */ 
// vim:tabstop=4:shiftwidth=4:noexpandtab:textwidth=80

// static
#include "static.h"
// xpdf
#include "xpdf.h"
//
#include "factories.h"
#include "cobject.h"
#include "cpdf.h"
#include "cxref.h"


// =====================================================================================
namespace pdfobjects{
// =====================================================================================

namespace
{

	/**
	 * Case insensitive comparator.
	 */
	bool nocase_compare (char c1, char c2)
	{
		return toupper(c1) == toupper(c2);
	}
}


//
// General object functions
//
// =====================================================================================
namespace utils {
// =====================================================================================

using namespace std;
using namespace xpdf;
using namespace boost;

//
// Forward declaration
//
void xpdfObjToString (Object& obj, string& str);

// =====================================================================================
namespace {
// =====================================================================================

	//
	// String constants used when converting objects to strings
	//
	// CObjectSimple
	/** Object Null string representation. */
	const string CNULL_NULL = "null";
		
	/** Object Bool false repersentation. */
	const string CBOOL_TRUE  = "true";
	/** Object Bool true representation. */
	const string CBOOL_FALSE = "false";

	/** Object Name string representation. */
	const string CNAME_PREFIX = "/";

	/** Object String representation prefix string. */
	const string CSTRING_PREFIX = "(";
	/** Object String representation suffix string. */
	const string CSTRING_SUFFIX = ")";

	/** Object Ref representation middle string. */
	const string CREF_MIDDLE = " ";
	/** Object Ref representation string suffix. */
	const string CREF_SUFFIX = " R";

	// CObjectComplex
	/** Object Array representation prefix string. */
	const string CARRAY_PREFIX = "[";
	/** Object Array representation middle string. */
	const string CARRAY_MIDDLE = " ";
	/** Object Array representation suffix string. */
	const string CARRAY_SUFFIX = " ]";

	/** Object Dictionary representation specifics. */
	const string CDICT_PREFIX = "<<";
	/** Object Dictionary representation specifics. */
	const string CDICT_MIDDLE = "\n/";
	/** Object Dictionary representation specifics. */
	const string CDICT_BETWEEN_NAMES = " ";
	/** Object Dictionary representation specifics. */
	const string CDICT_SUFFIX = "\n>>";

	/** Object Stream string representation specifics. */
	const string CSTREAM_HEADER = "\nstream\n";
	/** Object Stream string representation specifics. */
	const string CSTREAM_FOOTER = "\nendstream";

	/** Xpdf error object string representation specifics. */
	const string OBJERROR = "\n";

	/** Indirect Object heaser. */
	const string INDIRECT_HEADER = "obj ";
	/** Indirect Object footer. */
	const string INDIRECT_FOOTER = "\nendobj";

	/**
	 * Read processors for simple types.
	 */
	template<typename Storage, typename Val>
	struct xpdfBoolReader
	{
		public:
			void operator() (Storage obj, Val val)
			{
				if (objBool != obj.getType())
					throw ElementBadTypeException ("Xpdf object is not bool.");
			 	val = (0 != obj.getBool());
			}
	};

	/** \copydoc xpdfBoolReader */
	template<typename Storage, typename Val>
	struct xpdfIntReader
	{
		public:
			void operator() (Storage obj, Val val)
			{
				if (objInt != obj.getType())
					throw ElementBadTypeException ("Xpdf object is not int.");
			 	val = obj.getInt ();
			}
	};

	/** \copydoc xpdfBoolReader */
	template<typename Storage, typename Val>
	struct xpdfRealReader
	{
		public:
			void operator() (Storage obj, Val val)
			{
				if (!obj.isNum())
					throw ElementBadTypeException ("Xpdf object is not real.");
			 	val = obj.getNum ();
			}
	};

	/** 
	 * Reader for xpdf string objects.
	 * This functor enables conversion  from Storage type (xpdf string object)
	 * to given Val typed container. Val has to implement clear and append
	 * methods.
	 */
	template<typename Storage, typename Val>
	struct xpdfStringReader
	{
		public:
			/** Convers given xpdf string object to string stored to given val
			 * representation.
			 * @param obj Xpdf object to convert (must by objString).
			 * @param val Value storage output.
			 *
			 * Copies all bytes (including 0 bytes) from given object
			 * representing string value.
			 */
			void operator() (Storage obj, Val val)
			{
				// clear val content and add all bytes
				// getCString is not suitable because string may contain 0
				// characters
				val.clear();
				size_t len = obj.getString()->getLength();
				GString * xpdfString=obj.getString();
				for(size_t i=0; i< len; ++i)
				{
					char c = xpdfString->getChar(i);
					val += c;
				}
				assert (len == val.length());
			}
	};

	/** \copydoc xpdfBoolReader */
	template<typename Storage, typename Val>
	struct xpdfNameReader
	{
		public:
			void operator() (Storage obj, Val val)
			{
				if (objName != obj.getType())
					throw ElementBadTypeException ("Xpdf object is not name.");
			 	val = obj.getName ();
			}
	};

	/** \copydoc xpdfBoolReader */
	template<typename Storage, typename Val>
	struct xpdfRefReader
	{
		public:
			void operator() (Storage obj, Val val)
			{
				if (objRef != obj.getType())
					throw ElementBadTypeException ("Xpdf object is not ref.");
				val.num = obj.getRefNum();
				val.gen = obj.getRefGen();
			}
	};
	
	/**
	 * WriteProcessors.
	 *
	 * We know that Storage is xpdf Object and value type depends on each writer type
	 */
	template<typename Storage, typename Val>
	struct xpdfBoolWriter
	{
		public:
			Object* operator() (Storage obj, Val val)
			{
				return obj->initBool (GBool(val));
			}
	};

	/** \copydoc xpdfBoolWriter */
	template<typename Storage, typename Val>
	struct xpdfIntWriter
	{
		public:
			Object* operator() (Storage obj, Val val)
			{
				return obj->initInt (val);
			}
	};

	/** \copydoc xpdfBoolWriter */
	template<typename Storage, typename Val>
	struct xpdfRealWriter
	{
		public:
			Object* operator() (Storage obj, Val val)
			{
				return obj->initReal (val);
			}
	};

	/** \copydoc xpdfBoolWriter */
	template<typename Storage, typename Val>
	struct xpdfStringWriter
	{
		public:
			Object* operator() (Storage obj, Val val)
			{
				const char * str = val.c_str();
				return obj->initString (new GString(str, val.length()));
			}
	};

	/** \copydoc xpdfBoolWriter */
	template<typename Storage, typename Val>
	struct xpdfNameWriter
	{
		public:
			Object* operator() (Storage obj, Val val)
			{
				return obj->initName (const_cast<char*>(val.c_str()));
			}
	};

	/** \copydoc xpdfBoolWriter */
	template<typename Storage, typename Val>
	struct xpdfNullWriter
	{
		public:
			Object* operator() (Storage obj, Val)
			{
				return obj->initNull ();
			}
	};

	/** \copydoc xpdfBoolWriter */
	template<typename Storage, typename Val>
	struct xpdfRefWriter
	{
		public:
			Object* operator() (Storage obj, Val val)
			{
				return obj->initRef (val.num, val.gen);
			}
	};


	/*
	 * This type trait holds information which writer class to use with specific CObject class.
	 * REMARK: If we want to abandon using Object as information holder, you need to rewrite
	 * these classes to be able to write values.
	 * 
	 * One class can be used with more CObject classes.
	 */
	template<typename T, typename U, PropertyType Tp> struct ProcessorTraitSimple; 
	template<typename T, typename U> struct ProcessorTraitSimple<T,U,pBool>  
	{
		typedef struct utils::xpdfBoolWriter<T,U> xpdfWriteProcessor;
		typedef struct utils::xpdfBoolReader<T,U> xpdfReadProcessor;
	};
	template<typename T, typename U> struct ProcessorTraitSimple<T,U,pInt>	 
	{
		typedef struct utils::xpdfIntWriter<T,U> xpdfWriteProcessor;
		typedef struct utils::xpdfIntReader<T,U> xpdfReadProcessor;
	};
	template<typename T, typename U> struct ProcessorTraitSimple<T,U,pReal>  
	{
		typedef struct utils::xpdfRealWriter<T,U> xpdfWriteProcessor;
		typedef struct utils::xpdfRealReader<T,U> xpdfReadProcessor;
	};
	template<typename T, typename U> struct ProcessorTraitSimple<T,U,pString>
	{
		typedef struct utils::xpdfStringWriter<T,U> xpdfWriteProcessor;
		typedef struct utils::xpdfStringReader<T,U> xpdfReadProcessor;
	};
	template<typename T, typename U> struct ProcessorTraitSimple<T,U,pName>  
	{
		typedef struct utils::xpdfNameWriter<T,U> xpdfWriteProcessor;
		typedef struct utils::xpdfNameReader<T,U> xpdfReadProcessor;
	};
	template<typename T, typename U> struct ProcessorTraitSimple<T,U,pNull>   
	{
		typedef struct utils::xpdfNullWriter<T,U> xpdfWriteProcessor;
	};
	template<typename T, typename U> struct ProcessorTraitSimple<T,U,pRef>	 
	{
		typedef struct utils::xpdfRefWriter<T,U> xpdfWriteProcessor;
		typedef struct utils::xpdfRefReader<T,U> xpdfReadProcessor;
	};


	/**
	 * This object parses xpdf object to CArray.
	 */
	template<typename ObjectToParse, typename CObject>
	struct xpdfArrayReader
	{public:
		void operator() (IProperty& ip, const ObjectToParse array, CObject resultArray)
		{
			assert (objArray == array.getType());
			if (objArray != array.getType())
				throw ElementBadTypeException ("Array reader got xpdf object that is not array.");
			assert (0 <= array.arrayGetLength ());
			utilsPrintDbg (debug::DBG_DBG, "xpdfArrayReader\tobjType = " << array.getTypeName() );
			
			CPdf* pdf = ip.getPdf ();
			XpdfObject obj;

			int len = array.arrayGetLength ();
			for (int i = 0; i < len; ++i)
			{
				// Get Object at i-th position
				array.arrayGetNF (i, obj.get());
					
				shared_ptr<IProperty> cobj;
				// Create CObject from it
				if (isPdfValid(pdf))
				{
					hasValidRef (ip);
					cobj = shared_ptr<IProperty> (createObjFromXpdfObj (*pdf, *obj, ip.getIndiRef()));

				}else
				{
					cobj = shared_ptr<IProperty> (createObjFromXpdfObj (*obj));
				}

				if (cobj)
				{
					// Store it in the storage
					resultArray.push_back (cobj);
					// Free resources allocated by the object
					obj.reset ();
						
				}else
					throw CObjInvalidObject ();

			}	// for
		}	// void operator
	};

	/**
	 * This object parses xpdf object to CDict.
	 */
	template<typename ObjectToParse, typename CObject>
	struct xpdfDictReader
	{public:
		void operator() (IProperty& ip, const ObjectToParse dict, CObject resultDict)
		{
			assert (objDict == dict.getType());
			if (objDict != dict.getType())
				throw ElementBadTypeException ("Dict reader got xpdf object that is not dict.");
			assert (0 <= dict.dictGetLength ());
			utilsPrintDbg (debug::DBG_DBG, "xpdfDictReader\tobjType = " << dict.getTypeName() );
			
			CPdf* pdf = ip.getPdf ();
			xpdf::XpdfObject obj;

			int len = dict.dictGetLength ();
			for (int i = 0; i < len; ++i)
			{
				// Get Object at i-th position
				string key (dict.dictGetKey (i));
				obj->free ();
				dict.dictGetValNF (i,obj.get());

				shared_ptr<IProperty> cobj;
				// Create CObject from it
				if (isPdfValid (pdf))
					cobj = shared_ptr<IProperty> (createObjFromXpdfObj (*pdf, *obj, ip.getIndiRef()));
				else
					cobj = shared_ptr<IProperty> (createObjFromXpdfObj (*obj));

				if (cobj)
				{
					// Store it in the storage
					resultDict.push_back (make_pair(key,cobj));

				}else
					throw CObjInvalidObject ();
			}
		}
	};

	/*
	 * This type trait holds information which writer class to use with specific CObject class.
	 * REMARK: If we want to abandon using Object as information holder, you need to rewrite
	 * these classes to be able to write values.
	 * 
	 * One class can be used with more CObject classes.
	 */
	template<typename T, typename U, PropertyType Tp> struct ProcessorTraitComplex; 
	template<typename T, typename U> struct ProcessorTraitComplex<T,U,pArray>  
		{typedef struct utils::xpdfArrayReader<T,U>		xpdfReadProcessor;};
	template<typename T, typename U> struct ProcessorTraitComplex<T,U,pDict>   
		{typedef struct utils::xpdfDictReader<T,U>		xpdfReadProcessor;};


	/**
	 * Convert simple xpdf object to string.
	 *
	 * @param obj Object to parse.
	 * @param str Result string representation.
	 */
	void
	simpleXpdfObjToString (Object& obj,string& str)
	{
		utilsPrintDbg (debug::DBG_DBG," objType:" << obj.getTypeName() );

		ostringstream oss;

		switch (obj.getType()) 
		{
		case objBool:
			oss << ((obj.getBool()) ? CBOOL_TRUE : CBOOL_FALSE);
			break;

		case objInt:
			oss << obj.getInt();
			break;

		case objReal:
			oss << obj.getReal ();
			break;

		case objString:
			oss << CSTRING_PREFIX  << utils::makeStringPdfValid(obj.getString()) << CSTRING_SUFFIX;
			break;

		case objName:
			oss << CNAME_PREFIX << utils::makeNamePdfValid(obj.getName());
			break;

		case objNull:
			oss << CNULL_NULL;
			break;

		case objRef:
			oss << obj.getRefNum() << CREF_MIDDLE << obj.getRefGen() << CREF_SUFFIX;
			break;

		case objCmd:
			oss << obj.getCmd ();
			break;

		case objError:
			oss << OBJERROR;
			break;
			
		default:
			assert (!"Bad object passed to simpleXpdfObjToString.");
			throw XpdfInvalidObject (); 
			break;
		}

		// convert oss to string
		str = oss.str ();
	}

	/**
	 * Convert complex xpdf object to string.
	 *
	 * @param obj Object to parse.
	 * @param str Result string representation.
	 */
	void
	complexXpdfObjToString (Object& obj, string& str)
	{
		
		utilsPrintDbg (debug::DBG_DBG,"\tobjType = " << obj.getTypeName() );

		ostringstream oss;
		xpdf::XpdfObject o;
		int i;

		switch (obj.getType()) 
		{
		
		case objArray:
			oss << CARRAY_PREFIX;
			for (i = 0; i < obj.arrayGetLength(); ++i) 
			{
				oss << CARRAY_MIDDLE;
				obj.arrayGetNF (i,o.get());
				string tmp;
				xpdfObjToString (*o,tmp);
				oss << tmp;
				o.reset ();
			}
			oss << CARRAY_SUFFIX;
			break;

		case objDict:
			oss << CDICT_PREFIX;
			for (i = 0; i <obj.dictGetLength(); ++i) 
			{
				oss << CDICT_MIDDLE << obj.dictGetKey(i) << CDICT_BETWEEN_NAMES;
				obj.dictGetValNF(i, o.get());
				string tmp;
				xpdfObjToString (*o,tmp);
				oss << tmp;
				o.reset ();
			}
			oss << CDICT_SUFFIX;
			break;

		case objStream:
			obj.streamReset ();
			{
				Dict* dict = obj.streamGetDict ();
				assert (NULL != dict);
				o->initDict (dict);
				std::string str;
				complexXpdfObjToString (*o, str);
				oss << str;
			}
			
			oss << CSTREAM_HEADER;
			obj.streamReset ();
			{
			int c = 0;
			while (EOF != (c = obj.streamGetChar())) 
				oss << static_cast<string::value_type> (c);
			}
			obj.streamClose ();
			oss << CSTREAM_FOOTER;
			break;
		
		default:
			assert (false);	
			break;
		}

		// convert oss to string
		str = oss.str ();
	}

// =====================================================================================
} // anonymous namespace
// =====================================================================================


// =====================================================================================
//  Creation functions
// =====================================================================================

//
// Creates CObject from xpdf object.
// 
IProperty*
createObjFromXpdfObj (CPdf& pdf, Object& obj,const IndiRef& ref)
{
	switch (obj.getType ())
	{
		case objBool:
			return CBoolFactory::getInstance (pdf, ref, obj);

		case objInt:
			return CIntFactory::getInstance (pdf, ref, obj);

		case objReal:
			return CRealFactory::getInstance (pdf, ref, obj);

		case objString:
			return CStringFactory::getInstance (pdf, ref, obj);

		case objName:
			return CNameFactory::getInstance (pdf, ref, obj);

		case objNull:
			return CNullFactory::getInstance ();

		case objRef:
			return CRefFactory::getInstance (pdf, ref, obj);

		case objArray:
			return CArrayFactory::getInstance (pdf,ref,obj);

		case objDict:
			return CDictFactory::getInstance (pdf, ref, obj);

		case objStream:
			return new CStream (pdf,obj,ref);

		default:
			assert (!"Bad type.");
			throw ElementBadTypeException ("createObjFromXpdfObj: Xpdf object has bad type.");
			break;
	}
}

IProperty*
createObjFromXpdfObj (Object& obj)
{
	switch (obj.getType ())
	{
		case objBool:
			return CBoolFactory::getInstance  (obj);

		case objInt:
			return CIntFactory::getInstance  (obj);

		case objReal:
			return CRealFactory::getInstance  (obj);

		case objString:
			return CStringFactory::getInstance  (obj);

		case objName:
			return CNameFactory::getInstance  (obj);

		case objNull:
			return CNullFactory::getInstance  ();

		case objRef:
			return CRefFactory::getInstance  (obj);

		case objArray:
			return CArrayFactory::getInstance (obj);

		case objDict:
			return CDictFactory::getInstance  (obj);

		case objStream:
			return new CStream (obj);

		default:
			//assert (!"Bad type.");
			throw ElementBadTypeException ("createObjFromXpdfObj: Xpdf object has bad type.");
			break;
	}
}


//
//
//
template <PropertyType Tp,typename T>
Object* 
simpleValueToXpdfObj (T val)
{
	Object* obj = XPdfObjectFactory::getInstance ();
	
	typename ProcessorTraitSimple<Object*, T, Tp>::xpdfWriteProcessor wp;
	return wp (obj,val);
}

template Object* simpleValueToXpdfObj<pBool,const bool&> (const bool& val);
template Object* simpleValueToXpdfObj<pInt,const int&> (const int& val);
template Object* simpleValueToXpdfObj<pReal,const double&> (const double& val);
template Object* simpleValueToXpdfObj<pString,const string&> (const string& val);
template Object* simpleValueToXpdfObj<pName,const string&> (const string& val);
template Object* simpleValueToXpdfObj<pNull,const NullType&> (const NullType& val);
template Object* simpleValueToXpdfObj<pRef,const IndiRef&> (const IndiRef& val);

//
//
//
template <PropertyType Tp,typename T> 
void
simpleValueFromXpdfObj (Object& obj, T val)
{
	typename ProcessorTraitSimple<Object&, T, Tp>::xpdfReadProcessor rp;
	rp (obj,val);
}

//
// Special case for pNull
//
template <> 
inline void
simpleValueFromXpdfObj<pNull,NullType&> (Object&, NullType&) 
{
	/*assert (!"operation not permitted...");*//*THIS IS FORBIDDEN IN THE CALLER*/
}

template void simpleValueFromXpdfObj<pBool, bool&> (Object& obj,  bool& val);
template void simpleValueFromXpdfObj<pInt, int&> (Object& obj,  int& val);
template void simpleValueFromXpdfObj<pReal, double&> (Object& obj,  double& val);
template void simpleValueFromXpdfObj<pString, string&> (Object& obj,  string& val);
template void simpleValueFromXpdfObj<pName, string&> (Object& obj,  string&	val);
template void simpleValueFromXpdfObj<pNull, NullType&> (Object& obj,  NullType& val);
template void simpleValueFromXpdfObj<pRef, IndiRef&> (Object& obj,  IndiRef& val);

//
//
//
template <PropertyType Tp,typename T> 
inline void
complexValueFromXpdfObj (IProperty& ip, Object& obj, T val)
{
	typename ProcessorTraitComplex<Object&, T, Tp>::xpdfReadProcessor rp;
	rp (ip, obj, val);
}

template void complexValueFromXpdfObj<pArray, CArray::Value&> 
		(IProperty& ip, 
		 Object& obj, 
		 CArray::Value& val);

template void complexValueFromXpdfObj<pDict, CDict::Value&>
		(IProperty& ip, 
		 Object& obj, 
		 CDict::Value& val);

//
//
//
void
simpleValueFromString (const std::string& str, bool& val)
{
	static const string __true ("true");
	static const string __false ("false");
	
	if ( equal (str.begin(), str.end(), __true.begin(), nocase_compare))
		val = true;
	else if ( equal (str.begin(), str.end(), __false.begin(), nocase_compare)) 
		val = false;
	else
		throw CObjBadValue ();
}

void
simpleValueFromString (const std::string& str, int& val)
{
	std::stringstream ss (str);
	ss.exceptions (stringstream::failbit | stringstream::badbit);
	try {
		ss >> val;
	}catch (stringstream::failure& e) 
	{
		throw CObjBadValue ();
	}					
}

void
simpleValueFromString (const std::string& str, double& val)
{
	shared_ptr<Object> ptrObj (xpdfObjFromString (str), xpdf::object_deleter());
	
	assert (objReal == ptrObj->getType ());
	if (objReal != ptrObj->getType() && objInt != ptrObj->getType())
		throw CObjBadValue ();
					
	ProcessorTraitSimple<Object&, double&, pReal>::xpdfReadProcessor rp;
	rp (*ptrObj, val);
}

void
simpleValueFromString (const std::string& str, std::string& val)
{
	val = str;
}

void
simpleValueFromString (const std::string& str, IndiRef& val)
{
	std::stringstream ss (str);
	ss.exceptions (stringstream::failbit | stringstream::badbit);
	try {
		ss >> val.num;
		ss >> val.gen;
	}catch (stringstream::failure& e) 
	{
		throw CObjBadValue ();
	}
}

//
//
//
void 
dictFromXpdfObj (CDict& resultDict, ::Object& dict)
{
	assert (objDict == dict.getType());
	if (objDict != dict.getType())
			throw ElementBadTypeException ("");
	assert (0 <= dict.dictGetLength ());
	utilsPrintDbg (debug::DBG_DBG, "" );
			
	xpdf::XpdfObject obj;

	int len = dict.dictGetLength ();
	for (int i = 0; i < len; ++i)
	{
		// Make string from key
		string key = dict.dictGetKey (i);
		// Make IProperty from value
		dict.dictGetValNF (i,obj.get());
		scoped_ptr<IProperty> cobj (createObjFromXpdfObj (*obj));
		if (cobj)
		{
			// Store it in the dictionary
			resultDict.addProperty (key, *cobj);

		}else
			throw CObjInvalidObject ();
	}
	assert ((size_t)len == resultDict.getPropertyCount());
}


// FIXME remove this is duplication from xpdf code. We don't
// need it here (beside nasty complex CObject translation to 
// the xpdf Object based on CObject->string->Object)

//
//
//
::Object*
xpdfObjFromString (const std::string& str, XRef* xref)
{
	//utilsPrintDbg (debug::DBG_DBG,"xpdfObjFromString from " << str);
	//utilsPrintDbg (debug::DBG_DBG,"xpdfObjFromString size " << str.size());
	
	//
	// Create parser. It can create complex types. Lexer knows just simple types.
	// Lexer SHOULD delete MemStream.
	//
	// dct is freed in ~BaseStream
	//
	::Object dct;
	
	// xpdf MemStream frees buf 
	size_t len = str.size ();
	char* pStr = (char *)gmalloc(len + 1);
	memcpy(pStr, str.c_str(), len);
	pStr[len] = '\0';
					
	scoped_ptr<Parser> parser(
			new Parser (xref, 
				new Lexer (xref,
					new MemStream (pStr, 0, len, &dct, true)
					)
				)
			);
	//
	// Get xpdf obj from the stream
	//
	::Object* obj = XPdfObjectFactory::getInstance();
	parser->getObj (obj);

	//
	// If xpdf returned objNull and we did not give him null, an error occured
	//
	const static string null ("null");
	if ( (obj->isNull()) && !equal(str.begin(), str.end(), null.begin(), nocase_compare) )
	{
		obj->free ();
		utils::freeXpdfObject (obj);
		throw CObjBadValue ();
	}

	return obj;
}

//
//
//
::Object* 
xpdfStreamObjFromBuffer (const CStream::Buffer& buffer, const CDict& dict)
{
	STATIC_CHECK (1 == sizeof(CStream::Buffer::value_type), WANT_TO_READ_ONE_CHAR_BUT_GET_BUFFER_WITH_LARGER_STORAGE_THAN_CHAR);
	
	//
	// Copy buffer and use parser to make stream object
	//
	char* tmpbuf = static_cast<char*> (gmalloc (buffer.size() + CSTREAM_FOOTER.length()));
	size_t i = 0;
	for (CStream::Buffer::const_iterator it = buffer.begin(); it != buffer.end (); ++it)
		tmpbuf [i++] = static_cast<char> ((unsigned char)(*it));
	std::copy (CSTREAM_FOOTER.begin(), CSTREAM_FOOTER.end(), &(tmpbuf[i]));
	assert (i == buffer.size());
	//utilsPrintDbg (debug::DBG_DBG, tmpbuf);
	
	// Create stream
	::Object* objDict = dict._makeXpdfObject ();
	// Only undelying dictionary is used from objDict, so we can free objDict normally (this is 
	// due to the strange implementation of xpdf streams, no dict reference counting is used there
	::Stream* stream = new ::MemStream (tmpbuf, 0, buffer.size(), objDict, true);
	// Set filters
	stream = stream->addFilters (objDict);
	stream->reset ();
	::Object* obj = XPdfObjectFactory::getInstance ();
	obj->initStream (stream);
	
	// Free xpdf object that holds dictionary (not the dictionary itself)
	gfree (objDict);
	
	return obj;
}

// =====================================================================================
//  To/From string functions
// =====================================================================================


//
// To string functions
// 

//
//
//
void 
xpdfObjToString (Object& obj, string& str)
{
	switch (obj.getType())
	{

		case objArray:
		case objDict:
		case objStream:
			complexXpdfObjToString (obj,str);
			break;
		default:
			simpleXpdfObjToString (obj,str);
			break;
	}
}

//
// Convert simple value from CObjectSimple to string
//
template <>
void
simpleValueToString<pBool> (bool val, string& str)
{
	str = ((val) ? CBOOL_TRUE : CBOOL_FALSE);
}
template void simpleValueToString<pBool> (bool val, string& str);
//
//
//
template <>
void
simpleValueToString<pInt> (int val, string& str)
{
	char buf[24];
	sprintf(buf,"%d",val);
	str=buf;
}
template void simpleValueToString<pInt> (int val, string& str);
//
//
//
template <>
void
simpleValueToString<pReal> (double val, string& str)
{
	char buf[64];
	sprintf(buf,"%g",val);
	str.assign(buf);
}
template void simpleValueToString<pReal> (double val, string& str);
//
// Special case for pString and pName
//
template<PropertyType Tp>
void
simpleValueToString (const std::string& val, std::string& str)
{
	STATIC_CHECK ((pString == Tp) || (pName == Tp), COBJECT_INCORRECT_USE_OF_simpleObjToString_FUNCTION);

	switch (Tp)
	{
		case pString:
			str = CSTRING_PREFIX + makeStringPdfValid (val.begin(), val.end()) + CSTRING_SUFFIX;
			break;

		case pName:
			str = CNAME_PREFIX + makeNamePdfValid (val.begin(), val.end());
			break;

		default:
			assert (0);
			break;
	}
}

size_t stringToCharBuffer (Object & stringObject, CharBuffer & outputBuf)
{
using namespace std;
using namespace debug;

	utilsPrintDbg(DBG_DBG, "");
	if(stringObject.getType()!=objString)
	{
		utilsPrintDbg(DBG_ERR, "Given object is not a string. Object type="<<stringObject.getType());
		return 0;
	}

	// gets all bytes from xpdf string object, makes string valid (to some
	// escaping)
	::GString * s=stringObject.getString(); 
	string str;
	for(int i=0; i<s->getLength(); i++)
		str.append(1, s->getChar(i));
	string validStr;
	simpleValueToString<pString>(str, validStr);

	// creates buffer and fills it byte after byte from valid string
	// representation
	size_t len=validStr.length();
	char* buf = char_buffer_new (len);
	outputBuf = CharBuffer (buf, char_buffer_delete()); 
	for(size_t i=0; i<len; i++)
		buf[i]=validStr[i];
	
	return len;
}

template void simpleValueToString<pString> (const string& val, string& str);
template void simpleValueToString<pName> (const string& val, string& str);
//
// Special case for pNull
//
template<>
void
simpleValueToString<pNull> (const NullType&, string& str)
{
	str = CNULL_NULL;
}
template void simpleValueToString<pNull> (const NullType&, string& str);
//
// Special case for pRef
//
template<>
void
simpleValueToString<pRef> (const IndiRef& ref, string& str)
{
	ostringstream oss;
	oss << ref.num << CREF_MIDDLE << ref.gen << CREF_SUFFIX;
	// convert oss to string
	str = oss.str ();
}
template void simpleValueToString<pRef> (const IndiRef&, string& str);



//
// Convert complex value from CObjectComplex to string
//
template<>
void
complexValueToString<CArray> (const CArray::Value& val, string& str)
{
	utilsPrintDbg (debug::DBG_DBG,"complexValueToString<pArray>()" );
		
	ostringstream oss;

	// start tag
	str = CARRAY_PREFIX;
		
	//
	// Loop through all items and get each string and append it to the result
	//
	CArray::Value::const_iterator it = val.begin();
	for (; it != val.end(); ++it) 
	{
		str += CARRAY_MIDDLE;
		string tmp;
		(*it)->getStringRepresentation (tmp);
		str += tmp;
	}
		
	// end tag
	str += CARRAY_SUFFIX;
}
template void complexValueToString<CArray> (const CArray::Value& val, string& str);
//
//
//
template<>
void
complexValueToString<CDict> (const CDict::Value& val, string& str)
{
	utilsPrintDbg (debug::DBG_DBG,"complexValueToString<pDict>()");

	// start tag
	str = CDICT_PREFIX;

	//
	// Loop through all items and get each items name + string representation
	// and append it to the result
	//
	CDict::Value::const_iterator it = val.begin ();
	for (; it != val.end(); ++it) 
	{
		str += CDICT_MIDDLE + (*it).first + CDICT_BETWEEN_NAMES;
		string tmp;
		(*it).second->getStringRepresentation (tmp);
		str += tmp;
	}

	// end tag
	str += CDICT_SUFFIX;
}
template void complexValueToString<CDict> (const CDict::Value& val, string& str);

//
//
//
template<typename ITERATOR, typename OUTITERATOR>
void streamToString (const std::string& strDict, ITERATOR begin, ITERATOR end, OUTITERATOR out)
{
	// Insert dictionary
	std::copy (strDict.begin(), strDict.end(), out);
	
	// Insert header
	std::copy (CSTREAM_HEADER.begin(), CSTREAM_HEADER.end(), out);
	
	//Insert buffer
	std::copy (begin, end, out);

	// Insert footer
	std::copy (CSTREAM_FOOTER.begin(), CSTREAM_FOOTER.end(), out);
}
// Explicit initialization
template void streamToString<CStream::Buffer::const_iterator, std::back_insert_iterator<std::string> > 
	(const std::string& strDict, 
	 CStream::Buffer::const_iterator begin, 
	 CStream::Buffer::const_iterator end,
	 std::back_insert_iterator<std::string> out);

//
//
// TODO asIndirect parameter
size_t 
streamToCharBuffer (const std::string& strDict, const CStream::Buffer& streambuf, CharBuffer& outbuf)
{
	utilsPrintDbg (debug::DBG_DBG, "");
	
	// Calculate overall length and allocate buffer
	size_t len = strDict.length() + CSTREAM_HEADER.length() + streambuf.size() + CSTREAM_FOOTER.length();
	char* buf = char_buffer_new (len);
	outbuf = CharBuffer (buf, char_buffer_delete()); 
	// Make pdf representation
	streamToString (strDict, streambuf.begin(), streambuf.end(), buf);
	return len;
}


size_t streamToCharBuffer (Object & streamObject, Ref ref, CharBuffer & outputBuf, bool asIndirect)
{
using namespace std;
using namespace debug;

	utilsPrintDbg(DBG_DBG, "");
	if(streamObject.getType()!=objStream)
	{
		utilsPrintDbg(DBG_ERR, "Given object is not a stream. Object type="<<streamObject.getType());
		return 0;
	}
	
	// gets stream dictionary string at first
	string dict;
	Object streamDict;
	streamDict.initDict(streamObject.streamGetDict());
	xpdfObjToString(streamDict, dict);

	// indirect header is filled only if asIndirect flag is set
	// same way footer
	string header="";
	if(asIndirect)
	{
		ostringstream indirectHeader;
		indirectHeader << ref.num << " " << ref.gen << " " << INDIRECT_HEADER << "\n";
		header += indirectHeader.str();
	}
	string footer=(asIndirect)
		?INDIRECT_FOOTER
		:"";

	// gets buffer len from stream dictionary Length field
	Object lenghtObj;
	streamDict.getDict()->lookup("Length", &lenghtObj);
	if(!lenghtObj.isInt())
	{
		utilsPrintDbg(DBG_ERR, "Stream dictionary Length field is not int. type="<<lenghtObj.getType());
		lenghtObj.free();
		return 0;
	}
	if(!lenghtObj.getInt()<0)
	{
		utilsPrintDbg(DBG_ERR, "Stream dictionary Length field doesn't have correct value. value="<<lenghtObj.getInt());
		lenghtObj.free();
		return 0;
	}
	size_t bufferLen=(size_t)lenghtObj.getInt();
	lenghtObj.free();

	// gets total length and allocates CharBuffer for output	
	size_t len = 
		header.length() 
		+ dict.length()
		+ CSTREAM_HEADER.length() + bufferLen + CSTREAM_FOOTER.length() 
		+ footer.length(); 
	char* buf = char_buffer_new (len);
	outputBuf = CharBuffer (buf, char_buffer_delete()); 
	
	// get everything together into created buf - we can't use
	// str* functions here, because there may be \0 inside
	// dictionary string represenatation (string entries)
	size_t copied=0;
	memcpy(buf, header.c_str(), header.length());
	copied+=header.length();
	memcpy(buf+copied, dict.c_str(), dict.length());
	copied+=dict.length();
	memcpy(buf+copied, CSTREAM_HEADER.c_str(), CSTREAM_HEADER.length());
	copied+=CSTREAM_HEADER.length();
	// streamBuffer may contain '\0' so rest has to be copied by memcpy
	BaseStream * base=streamObject.getStream()->getBaseStream();
	base->reset();
	size_t i=0;
	for(; i<bufferLen; i++)
	{
		int ch=base->getChar();
		if(ch==EOF)
			break;
		// NOTE that we are reducing int -> char here, so multi-bytes
		// returned from a stream will be stripped to 1B
		if((unsigned char)ch != ch)
			utilsPrintDbg(DBG_DBG, "Too wide character("<<ch<<") in the stream at pos="<<i);
		buf[copied+i]=ch;
	}
	// checks number of written bytes and if any problem (more or less data),
	// clears buffer and returns with 0
	if(i!=bufferLen)
	{
		utilsPrintDbg(DBG_ERR, "Unexpected end of stream. "<<i<<" bytes read.");
		// TODO do we really need to clear whole buffer here? Is it enough to clear
		// the first byte?
		memset(buf, '\0', len);
		streamObject.getStream()->reset();
		return 0;
	}
	if(base->getChar()!=EOF)
	{
		utilsPrintDbg(DBG_ERR, "stream contains more data than claimed by dictionary Length field.");
		// TODO do we really need to clear whole buffer here? Is it enough to clear
		// the first byte?
		memset(buf, '\0', len);
		streamObject.getStream()->reset();
		return 0;
	}
	copied+=bufferLen;
	memcpy(buf + copied, CSTREAM_FOOTER.c_str(), CSTREAM_FOOTER.length());
	copied+=CSTREAM_FOOTER.length();
	memcpy(buf + copied, footer.c_str(), footer.length());
	copied+=footer.length();
	
	// just to be sure
	assert(copied==len);
	
	// restore stream object to the begining
	streamObject.getStream()->reset();
	return len;
}


//
//
//
void
getStringFromXpdfStream (std::string& str, ::Object& obj)
{
	if (!obj.isStream())
	{
		assert (!"Object is not stream.");
		throw XpdfInvalidObject ();
	}

	// Clear string
	str.clear ();

	// Reset stream
	obj.streamReset ();
	// Save chars
	int c;
	while (EOF != (c = obj.streamGetChar())) 
		str += static_cast<std::string::value_type> (c);
	// Cleanup
	obj.streamClose ();
}


//
//
//
void 
createIndirectObjectStringFromString  ( const IndiRef& rf, const std::string& val, std::string& output)
{
	ostringstream oss;

	oss << rf.num << " " << rf.gen << " " << INDIRECT_HEADER << "\n";
	oss << val;
	oss << INDIRECT_FOOTER;

	output = oss.str ();
}


// =====================================================================================
//  Other functions
// =====================================================================================

//
//
//
void
freeXpdfObject (Object* obj)
{
	assert (obj != NULL);
	if (NULL == obj)
		throw XpdfInvalidObject ();
	
	// delete all member variables
	obj->free ();
	// delete the object itself
	gfree(obj);
}




//
//
//
bool
objHasParent (const IProperty& ip, boost::shared_ptr<IProperty>& indiObj)
{
	assert (hasValidPdf (ip));
	if (!hasValidPdf (ip))
		throw CObjInvalidOperation ();

	CPdf* pdf = ip.getPdf ();
	IndiRef ref = ip.getIndiRef();
	if ( &ip == (indiObj=pdf->getIndirectProperty(ref)).get() )
		return false;
	else
		return true;
}
bool
objHasParent (const IProperty& ip)
{boost::shared_ptr<IProperty> indi;return objHasParent (ip,indi);}


	
//
//
//
template<typename T>
void 
parseStreamToContainer (T& container, ::Object& obj)
{
	assert (container.empty());
	if (!obj.isStream())
	{
		assert (!"Object is not stream.");
		throw XpdfInvalidObject ();
	}

	// Get stream length
	xpdf::XpdfObject xpdfDict; xpdfDict->initDict (obj.streamGetDict());
	xpdf::XpdfObject xpdfLen; xpdfDict->dictLookup ("Length", xpdfLen.get());
	assert (xpdfLen->isInt ());
	assert (0 <= xpdfLen->getInt());
	size_t len = static_cast<size_t> (xpdfLen->getInt());
	utilsPrintDbg (debug::DBG_DBG, "Stream length: " << len);
	// Get stream
	::Stream* xpdfStream = obj.getStream ();
	assert (xpdfStream);
	// Get base stream without filters
	xpdfStream->getBaseStream()->moveStart (0);
	Stream* rawstr = xpdfStream->getBaseStream();
	assert (rawstr);
	// \TODO THIS IS MAGIC (try-fault practise)
	rawstr->reset ();

	// Save chars
	int c;
	while (container.size() < len && EOF != (c = rawstr->getChar())) 
		container.push_back (static_cast<typename T::value_type> (c));
	
	utilsPrintDbg (debug::DBG_DBG, "Container length: " << container.size());
	
	if (len != container.size())
		utilsPrintDbg(debug::DBG_ERR, "Stream buffer length ("<<container.size()<<") doesn't match Length value ("<<len<<").")

	assert (len == container.size());
	// Cleanup
	obj.streamClose ();
	//\TODO is it really ok?
	rawstr->close ();
}
template void parseStreamToContainer<CStream::Buffer> (CStream::Buffer& container, ::Object& obj);

	



// =====================================================================================
} /* namespace utils */
// =====================================================================================
} /* namespace pdfobjects */
// =====================================================================================
