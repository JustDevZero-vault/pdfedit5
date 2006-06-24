// vim:tabstop=4:shiftwidth=4:noexpandtab:textwidth=80
/* 
 * =====================================================================================
 *        Filename:  cstream.h
 *         Created:  01/18/2006 
 *          Author:  jmisutka (06/01/19), 
 *
 * =====================================================================================
 */
#ifndef _CSTREAM_H
#define _CSTREAM_H

// all basic includes
#include "static.h"
#include "cdict.h"
#include "iproperty.h"
// Filters
#include "filters.h"


//=====================================================================================
namespace pdfobjects {
//=====================================================================================


//=====================================================================================
// CStream
//=====================================================================================

//
// Forward declaration
//
template<typename T> class CStreamsXpdfReader;
namespace utils { template<typename Iter> void makeStreamPdfValid (Iter it, Iter end, std::string& out); }

/**
 * Template class representing stream PDF objects from specification v1.5.
 *
 * It is neither a simple object, because it does not
 * contain just simple value, nor a complex object, because it can not be simple represented
 * in that generic class. It contains a dictionary and a stream. It does not have methods common
 * to complex objects.
 *
 * This is a generic class joining implementation of dictionary and array together in in one place.
 *
 * Other xpdf objects like objCmd can not be instantiated although the PropertyType 
 * exists. It is because PropertyTraitSimple is not specialized for these types.
 *
 * We use memory checking with this class which save information about existing IProperties.
 * This technique can be used to detect memory leaks etc. 
 *
 * Xpdf stream objects are the worst from all xpdf objects because of their deallocation politics.
 * It is really not easy to say when, where and who should deallocate an xpdf stream, its buffer etc...
 * 
 * This class does not provide default copy constructor because copying a
 * property could be understood either as deep copy or shallow copy. 
 * Copying complex types could be very expensive so we have made this decision to
 * avoid it.
 */
class CStream : noncopyable, public IProperty
{
	typedef boost::iostreams::filtering_streambuf<boost::iostreams::output> filtering_ostreambuf;
	typedef std::string PropertyId;

public:
	// We can access xpdf stream only through CStreamsXpdfReader 
	template<typename T> friend class CStreamsXpdfReader;

	typedef std::vector<filters::StreamChar> Buffer;
	typedef observer::BasicChangeContext<IProperty> BasicObserverContext;

	/** Type of the property.
	 * This fields holds pStream value. It is used by template functions to get to
	 * property type from template type.
	 */
	static const PropertyType type=pStream;
protected:

	/** Object dictionary. */
	CDict dictionary;
	
	/** Buffer. */
	Buffer buffer;

	/** XpdfParser. */
	::Parser* parser;
	/** Current object in an opened stream. */
	mutable ::Object curObj;
		

	//
	// Constructors
	//
public:
	/**
	 * Constructor. Only kernel can call this constructor
	 *
	 * @param p		Pointer to pdf object.
	 * @param o		Xpdf object. 
	 * @param rf	Indirect id and gen id.
	 */
	CStream (CPdf& p, Object& o, const IndiRef& rf);

	/**
	 * Constructor. Only kernel can call this constructor
	 *
	 * @param o		Xpdf object. 
	 */
	CStream (Object& o);


public:	
	
	/** 
	 * Public constructor. This object will not be associated with a pdf.
	 * It adds one required property to objects dictionary namely "Length". This
	 * is according to the pdf specification.
	 *
	 * @param makeReqEntries If true required entries are added to stream
	 * dictionary.
	 */
	CStream (bool makeReqEntries = true);

	
	//
	// Cloning
	//
protected:

	/**
     * Implementation of clone method. 
	 *
     * @param Deep copy of this object.
	 */
	virtual IProperty* doClone () const;

	/** Return new instance. */
	virtual CStream* _newInstance () const
		{ return new CStream (false); }

	/** Create required entries. */
	void createReqEntries ();

	//
	// Dictionary methods, delegated to CDict
	//
public:
	/** Delagate this operation to underlying dictionary. \see CDict */
	size_t getPropertyCount () const
		{return dictionary.getPropertyCount ();}
	
	/** Delagate this operation to underlying dictionary. \see CDict */
	template<typename Container> 
	void getAllPropertyNames (Container& container) const
		{ dictionary.getAllPropertyNames (container); }
	
	/** Delagate this operation to underlying dictionary. \see CDict */
	boost::shared_ptr<IProperty> getProperty (PropertyId id) const
		{return dictionary.getProperty (id);}
	
	/** Delagate this operation to underlying dictionary. \see CDict */
	PropertyType getPropertyType (PropertyId id) const
		{return dictionary.getPropertyType (id);}
	
	/** Delagate this operation to underlying dictionary. \see CDict */
	boost::shared_ptr<IProperty> setProperty (PropertyId id, IProperty& ip)
		{return dictionary.setProperty (id, ip);}
	
	/** Delagate this operation to underlying dictionary. \see CDict */
	boost::shared_ptr<IProperty> addProperty (PropertyId id, const IProperty& newIp)
		{return dictionary.addProperty (id, newIp);}
	
	/** Delagate this operation to underlying dictionary. \see CDict */
	void delProperty (PropertyId id)
		{dictionary.delProperty (id);}


	//
	// Get methods
	//
public:	
	
	/** 
     * Returns type of object. 
     *
     * @return Type of this class.
     */
    virtual PropertyType getType () const {return pStream;}

	/**
	 * Returns string representation of this object.
	 *
	 * @param str Output string representation.
	 */
	virtual void getStringRepresentation (std::string& str) const;

	/**
	 * Returns decoded string representation of this object.
	 *
	 * @param str Output string representation.
	 */
	virtual void getDecodedStringRepresentation (std::string& str) const;

	/**
	 * Get encoded buffer. Can contain non printable characters.
	 *
	 * @return Buffer.
	 */
	const Buffer& getBuffer () const {return buffer;}
	
	/**
	 * Get filters.
	 *
	 * @param container Container of filter names.
	 */
	template<typename Container>
	void getFilters (Container& container) const
	{
		boost::shared_ptr<IProperty> ip;
		//
		// Get optional value Filter
		//
		try	
		{
			ip = dictionary.getProperty ("Filter");
			
		}catch (ElementNotFoundException&)
		{
			// No filter found
			kernelPrintDbg (debug::DBG_DBG, "No filter found.");
			return;
		}

		//
		// If it is a name just store it
		// 
		if (isName (ip))
		{
			boost::shared_ptr<const CName> name = IProperty::getSmartCObjectPtr<CName>(ip);
			container.push_back (name->getValue());
			
			kernelPrintDbg (debug::DBG_DBG, "Filter name:" << name->getValue());
		//
		// If it is an array, iterate through its properties
		//
		}else if (isArray (ip))
		{
			boost::shared_ptr<CArray> array = IProperty::getSmartCObjectPtr<CArray>(ip);
			// Loop throug all children
			typedef std::vector<boost::shared_ptr<IProperty> > Names;
			Names names;
			array->_getAllChildObjects (names);
			
			Names::iterator it = names.begin ();
			for (; it != names.end(); ++it)
			{
				boost::shared_ptr<CName> name = IProperty::getSmartCObjectPtr<CName>(*it);
				container.push_back (name->getValue());
				kernelPrintDbg (debug::DBG_DBG, "Filter name:" << name->getValue());
			}
		}
	}



	//
	// Set methods
	//
public:
	/**
	 * Set pdf to itself and also tu all children
	 *
	 * @param pdf New pdf.
	 */
	virtual void setPdf (CPdf* pdf);

	/**
	 * Set ref to itself and also tu all children
	 *
	 * @param pdf New indirect reference numbers.
	 */
	virtual void setIndiRef (const IndiRef& rf);

	/**
	 * Set encoded buffer.
	 *
	 * @param buf New buffer.
	 */
	void setRawBuffer (const Buffer& buf);

	/**
	 * Set decoded buffer. 
	 * Use avaliable filters. If a filter is not avaliable an exception is thrown.
	 *
	 * @param buf New buffer.
	 */
	template<typename Container>
	void setBuffer (const Container& buf)
	{
		kernelPrintDbg (debug::DBG_DBG, "");
		assert (NULL == parser || !"Stream is open.");
		if (NULL != parser)
			throw CObjInvalidOperation ();

		// Create context
		boost::shared_ptr<ObserverContext> context (this->_createContext());
		
		// Make buffer pdf valid, encode buf and save it to buffer
		std::string strbuf;
		utils::makeStreamPdfValid (buf.begin(), buf.end(), strbuf);
		encodeBuffer (strbuf);
		// Change length
		setLength (buffer.size());
		
		//Dispatch change 
		_objectChanged (context);
	}

	//
	// Parsing (use friend CStreamsXpdfReader)
	//
private:
	
	/**
	 * Initialize parsing mechanism.
	 *
	 * REMARK: if CObject is not in a pdf, we MUST be sure that it does not
	 * use indirect objects.
	 */
	void open ();

	/**
	 * Close parser.
	 */
	void close ();
	
	/**
	 * Get xpdf object and copy it to obj.
	 *
	 * REMARK: We can not do any buffering (caching) of xpdf objects, because
	 * xpdf already does caching and it will NOT work correctly with inline
	 * images. We would buffer WRONG data.
	 *
	 * @param obj Next xpdf object.
	 */
	void getXpdfObject (::Object& obj);
	
	/**
	 * Get xpdf stream. Be carefull this is not a copy.
	 * 
	 * @return Stream.
	 */
	 ::Stream* getXpdfStream ();

	/**
	 * Is the last object end of stream.
	 *
	 * This is not very common behaviour, but we can not use caching 
	 * \see getXpdfObject
	 * so we can tell if it is the end after fetching an object which means
	 * after calling getXpdfObject.
	 * 
	 * @return True if we no more data avaliable, false otherwise.
	 */
	bool eof () const;
	
	
	//
	// Destructor
	//
public:

	/**
	 * Destructor.
	 */
	~CStream ();



	//
	// Helper methods
	//
public:
	/**
	 * Make xpdf Object from this object. This function allocates and initializes xpdf object.
	 * Caller has to free the xpdf Object (call Object::free and then
	 * deallocating)
	 *
	 * \exception ObjBadValueE Thrown when xpdf can't parse the string representation of this
	 * object correctly.
	 * 
	 * @return Xpdf object representing value of this simple object.
	 */
	virtual Object* _makeXpdfObject () const; 


private:
	/**
	 * Indicate that the object has changed.
	 * Notifies all observers associated with this property about the change.
	 *
	 * @param context Context in which a change occured.
	 */
	void _objectChanged (boost::shared_ptr<const ObserverContext> context);

	/**
	 * Get encoded buffer.
	 *
	 * @param container Output container.
	 */
	template<typename Container>
	void encodeBuffer (const Container& container)
	{
		kernelPrintDbg (debug::DBG_DBG, "");

		//
		// Create input filtes and add filters according to Filter item in
		// stream dictionary
		// 
		filters::InputStream in;
		std::vector<std::string> filters;
		getFilters (filters);
		
		// Try adding filters if one is not supported use none
		try {
			
			filters::CFilterFactory::addFilters (in, filters);
		
		}catch(FilterNotSupported&)
		{
			kernelPrintDbg (debug::DBG_DBG, "One of the filters is not supported, using none..");
			
			dictionary.delProperty ("Filter");
			// Clear buffer
			buffer.clear ();
			copy (container.begin(), container.end(), back_inserter (buffer));
			return;
		}
		
		// Clear buffer
		buffer.clear ();

		// Create input source from buffer
		boost::iostreams::stream<filters::buffer_source<Container> > input (container);
		in.push (input);
		// Copy it to container
		Buffer::value_type c;
		in.get (c);
		while (!in.eof())
		{//\TODO Improve this !!!!
			buffer.push_back (c);
			in.get (c);
		}
		// Close the stream
		in.reset ();
	}


private:
	/**
	 * Get length.
	 *
	 * @return Stream length.
	 */
	size_t getLength () const;

	/**
	 * Set length.
	 *
	 * @param len Stream Length.
	 */
	void setLength (size_t len);

private:
	/**
	 * Create context of a change.
	 *
	 * REMARK: Be carefull. Deallocate this object.
	 * 
	 * @return Context in which a change occured.
	 */
	ObserverContext* _createContext () const;

public:
	/**
	 * Return all object we have access to.
	 *
	 * @param store Container of objects.
	 */
	template <typename Storage>
	void _getAllChildObjects (Storage& store) const
	{
		CDict::Value::const_iterator it = dictionary.value.begin ();
		for	(; it != dictionary.value.end (); ++it)
			store.push_back ((*it).second);
	}

	
	//
	// Special functions
	//
public:

	/**
	 * Return the list of all supported streams
	 *
	 * @return List of supported stream filters.
	 */
	template<typename Container>
	static void getSupportedStreams (Container& supported) 
		{ filters::CFilterFactory::getSupportedStreams (supported); }

};


//=====================================================================================
namespace utils {
//=====================================================================================

/**
 * Make stream pdf valid.
 *
 * Not needed now.
 * 
 * @param it Start insert iterator.
 * @param end End iterator.
 */
template<typename Iter>
void
makeStreamPdfValid (Iter it, Iter end, std::string& out)
{
	for (; it != end; ++it)
	{
		//if ( '\\' == (*it))
		//{ // "Escape" every occurence of '\'
		//		out += '\\';
		//}

		out += *it;
	}
}



/**
 * Parse stream object to a container
 *
 * @param container Container of characters (e.g. ints).
 * @param obj Stream object.
 */
template<typename T>
void parseStreamToContainer (T& container, ::Object& obj);


	
/**
 * Create xpdf object from string.
 *
 * @param buffer Stream buffer.
 * @param dict Stream dictionary.
 *
 * @return Xpdf object.
 */
::Object* xpdfStreamObjFromBuffer (const CStream::Buffer& buffer, const CDict& dict);


/**
 * CStream object to string
 *
 * @param strDict Dictionary string representation.
 * @param begin Buffer begin
 * @param end Buffer end
 * @param out Output string.
 */
template<typename ITERATOR, typename OUTITERATOR>
void streamToString (const std::string& strDict, ITERATOR begin, ITERATOR end, OUTITERATOR out);

/**
 * Makes a valid pdf representation of a stream using streamToString function.
 * 
 * @param strDict Dictionary string representation.
 * @param streambuf Raw stream buffer.
 * @param outbuf Output buffer.
 *
 * @param Length of data.
 */
size_t streamToCharBuffer (const std::string& strDict, const CStream::Buffer& streambuf, CharBuffer& outbuf);

//=========================================================
//	CDict "get type" helper methods
//=========================================================


/** 
 * Get stream from dictionary. If it is CRef fetch the object pointed at.
 */
template<typename IP>
inline boost::shared_ptr<CStream>
getCStreamFromDict (IP& ip, const std::string& key)
	{return getTypeFromDictionary<CStream> (ip, key);}

//=========================================================
//	CArray "get type" helper methods
//=========================================================

/** 
 * Get stream from array. If it is CRef fetch the object pointed at.
 */
template<typename IP>
inline boost::shared_ptr<CStream>
getCStreamFromArray (IP& ip, size_t pos)
	{return getTypeFromArray<CStream> (ip, pos);}




//=====================================================================================
} /* namespace utils*/
//=====================================================================================
} /* namespace pdfobjects */
//=====================================================================================


#endif // _CSTREAM_H

