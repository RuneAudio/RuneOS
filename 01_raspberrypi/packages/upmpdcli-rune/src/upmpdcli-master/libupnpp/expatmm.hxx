/*
 * ExpatMM - C++ Wrapper for Expat available at http://expat.sourceforge.net/
 * Copyright (c) 2006, 2007, 2008, 2009 IntelliTree Solutions llc
 * Author: Coleman Kane <ckane@intellitree.com>
 *
 * Mutilated and forced into single-file solution by <jf@dockes.org>
 * Copyright (c) 2013 J.F. Dockes
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, please feel free
 * to contact the author listed above.
 */
#ifndef _EXPATMM_EXPATXMLPARSER_H
#define _EXPATMM_EXPATXMLPARSER_H

#include <string.h>
#include <expat.h>

namespace expatmm {
	class ExpatXMLParser {
	public:

	/* Create a new parser, using the default Chunk Size */
	ExpatXMLParser(void)
	{
		init();
	}

	/* Create a new parser, using a user-supplied chunk size */
	ExpatXMLParser(size_t chunk_size)
	{
		init(chunk_size);
	}

	/* Destructor that cleans up xml_buffer and parser */
	virtual ~ExpatXMLParser(void)
	{
		valid_parser = false;
		if(expat_parser != NULL) {
			XML_ParserFree(expat_parser);
			expat_parser = NULL;
		}
		if(xml_buffer != NULL) {
			delete xml_buffer;
			xml_buffer = NULL;
		}
	}

		/*
	  Generic Parser. Most derivations will simply call this, rather
	  than implement their own. This will loop, processing XML data
	  and calling the necessary handler code until an error is encountered.
	*/
	virtual bool Parse(void)
	{
		/* Ensure that the parser is ready */
		if(!Ready())
			return false;

		ssize_t bytes_read;
		/* Loop, reading the XML source block by block */
		while((bytes_read = read_block()) >= 0) {
			if(bytes_read > 0) {
				XML_Status local_status =
					XML_Parse(expat_parser, getReadBuffer(), bytes_read,
							  XML_FALSE);

				if(local_status != XML_STATUS_OK) {
					status = local_status;
					last_error = XML_GetErrorCode(expat_parser);
					break;
				}

				/* Break on successful "short read", in event of EOF */
				if(getLastError() == XML_ERROR_FINISHED)
					break;
			}
		}

		/* Finalize the parser */
		if((getStatus() == XML_STATUS_OK) ||
		   (getLastError() == XML_ERROR_FINISHED)) {
			XML_Parse(expat_parser, getBuffer(), 0, XML_TRUE);
			return true;
		}

		/* Return false in the event of an error. The parser is
		   not finalized on error. */
		return false;
	}

	/* Expose status, error, and control codes to users */
	virtual bool Ready(void) { return valid_parser; };
	virtual XML_Error getLastError(void) { return last_error; };
	virtual XML_Status getStatus(void) { return status; };

	protected:
	virtual XML_Char *getBuffer(void) { return xml_buffer; };
	virtual const char *getReadBuffer(void) { return xml_buffer; };
	virtual size_t getBlockSize(void) { return xml_buffer_size; };

	/* Read XML data.
	 *
	 * Override this to implement your container-specific parser.
	 *
	 * You must:
	 * put new XML data into xml_buffer
	 * set status
	 * set last_error
	 * return the amount of XML_Char's written to xml_buffer
	 *
	 * on error, return < 0. The contents of xml_buffer will be
	 * thrown away in this event, so it is the derived class's
	 * responsibility to reseek the "data cursor" to re-get any
	 * data in the buffer on an error condition.
	 *
	 * Use setReadiness, setStatus, and setLastError to handle
	 * error, status, and control events and codes.
	 *
	 * The default implementation returns "no elements" if it is
	 * ever called. and should be overridden by the derived class.
	 *
	 * Note that, as the actual parser only uses
	 * getBuffer()/getBlockSize()/read_block() (no direct access
	 * to the buffer), you are free to use an entirely different
	 * I/O mechanism, like what does the inputRefXMLParser below.
	 */
	virtual ssize_t read_block(void)
	{
		last_error = XML_ERROR_NO_ELEMENTS;
		status = XML_STATUS_ERROR;
		return -1;
	}

	virtual void setReadiness(bool ready)
	{
		valid_parser = ready;
	}
	virtual void setStatus(XML_Status new_status)
	{
		status = new_status;
	}
	virtual void setLastError(XML_Error new_last_error)
	{
		last_error = new_last_error;
	};

	/* Methods to be overriden */
	virtual void StartElement(const XML_Char *,
				  const XML_Char **) {}
	virtual void EndElement(const XML_Char *) {}
	virtual void CharacterData(const XML_Char *, int) {}
	virtual void ProcessingInstruction(const XML_Char *,
					   const XML_Char *) {}
	virtual void CommentData(const XML_Char *) {}
	virtual void DefaultHandler(const XML_Char *, int) {}
	virtual void CDataStart(void) {}
	virtual void CDataEnd(void) {}

	private:
	/* The handle for the parser (expat) */
	XML_Parser expat_parser;

	/* Temporary buffer where data is streamed in */
	XML_Char *xml_buffer;
	size_t xml_buffer_size;

	/* Tells if the parser is ready to accept data */
	bool valid_parser;

	/* Status and Error codes in the event of unforseen events */
	XML_Status status;
	XML_Error last_error;

	/* Expat callbacks.
	 * The expatmm protocol is to pass (this) as the userData argument
	 * in the XML_Parser structure. These static methods will convert
	 * handlers into upcalls to the instantiated class's virtual members
	 * to do the actual handling work. */
	static void _element_start_handler(void *userData, const XML_Char *name,
					   const XML_Char **atts)
	{
		ExpatXMLParser *me = (ExpatXMLParser*)userData;
		if(me != NULL) me->StartElement(name, atts);
	}
	static void _element_end_handler(void *userData, const XML_Char *name)
	{
		ExpatXMLParser *me = (ExpatXMLParser*)userData;
		if(me != NULL) me->EndElement(name);
	}
	static void _character_data_handler(void *userData,
						const XML_Char *s, int len)
	{
		ExpatXMLParser *me = (ExpatXMLParser*)userData;
		if(me != NULL) me->CharacterData(s, len);
	}
	static void _processing_instr_handler(void *userData,
						  const XML_Char *target,
						  const XML_Char *data)
	{
		ExpatXMLParser *me = (ExpatXMLParser*)userData;
		if(me != NULL) me->ProcessingInstruction(target, data);
	}
	static void _comment_handler(void *userData, const XML_Char *data)
	{
		ExpatXMLParser *me = (ExpatXMLParser*)userData;
		if(me != NULL) me->CommentData(data);
	}
	static void _default_handler(void *userData, const XML_Char *s, int len)
	{
		ExpatXMLParser *me = (ExpatXMLParser*)userData;
		if(me != NULL) me->DefaultHandler(s, len);
	}
	static void _cdata_start_handler(void *userData)
	{
		ExpatXMLParser *me = (ExpatXMLParser*)userData;
		if(me != NULL) me->CDataStart();
	}
	static void _cdata_end_handler(void *userData)
	{
		ExpatXMLParser *me = (ExpatXMLParser*)userData;
		if(me != NULL) me->CDataEnd();
	}
		/* Register our static handlers with the Expat events. */
	void register_default_handlers()
	{
		XML_SetElementHandler(expat_parser, &_element_start_handler,
				  &_element_end_handler);
		XML_SetCharacterDataHandler(expat_parser, &_character_data_handler);
		XML_SetProcessingInstructionHandler(expat_parser,
						&_processing_instr_handler);
		XML_SetCommentHandler(expat_parser, &_comment_handler);
		XML_SetCdataSectionHandler(expat_parser, &_cdata_start_handler,
					   &_cdata_end_handler);
		XML_SetDefaultHandler(expat_parser, &_default_handler);
	}
	/* Constructor common code */
	void init(size_t chunk_size = 0)
	{
		valid_parser = false;
		expat_parser = NULL;
		xml_buffer_size = chunk_size ? chunk_size : 10240;
		xml_buffer = new XML_Char[xml_buffer_size];
		if(xml_buffer == NULL)
			return;
		expat_parser = XML_ParserCreate(NULL);

		if(expat_parser == NULL) {
			delete xml_buffer;
			xml_buffer = NULL;
			return;
		}
		status = XML_STATUS_OK;
		last_error = XML_ERROR_NONE;

		memset(xml_buffer, 0, chunk_size * sizeof(XML_Char));

		/* Set the "ready" flag on this parser */
		valid_parser = true;
		XML_SetUserData(expat_parser, (void*)this);
		register_default_handlers();
	}
	};

    /** A specialization of ExpatXMLParser that does not copy its input */
	class inputRefXMLParser : public ExpatXMLParser {
	public:
		// Beware: we only use a ref to input to minimize copying. This means
		// that storage for the input parameter must persist until you are done
		// with the parser object !
		inputRefXMLParser(const std::string& input)
			: ExpatXMLParser(1), // Have to allocate a small buf even if not used.
			  m_input(input)
			{}

	protected:
		ssize_t read_block(void)
			{
				if (getLastError() == XML_ERROR_FINISHED) {
					setStatus(XML_STATUS_OK);
					return -1;
				}
				setLastError(XML_ERROR_FINISHED);
				return m_input.size();
			}
		const char *getReadBuffer()
			{
				return m_input.c_str();
			}
		virtual size_t getBlockSize(void)
			{
				return m_input.size();
			}
	private:
		const std::string& m_input;
	};

}; // End namespace expatmm

#endif /* _EXPATMM_EXPATXMLPARSER_H */
/* Local Variables: */
/* mode: c++ */
/* c-basic-offset: 4 */
/* tab-width: 4 */
/* indent-tabs-mode: t */
/* End: */
