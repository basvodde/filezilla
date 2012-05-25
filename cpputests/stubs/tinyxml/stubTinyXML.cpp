
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "../tinyxml/tinyxml.h"

bool TiXmlBase::condenseWhiteSpace = true;

TiXmlAttributeSet::TiXmlAttributeSet()
{
	FAIL("TiXmlAttributeSet::TiXmlAttributeSet");
}

TiXmlAttributeSet::~TiXmlAttributeSet()
{
	FAIL("TiXmlAttributeSet::~TiXmlAttributeSet");
}

TiXmlNode::~TiXmlNode()
{
	FAIL("TiXmlNode::~TiXmlNode")
}

TiXmlNode::TiXmlNode( NodeType _type )
{
	FAIL("TiXmlNode::TiXmlNode");
}

void TiXmlNode::Clear()
{
	FAIL("TiXmlNode::Clear");
}

TiXmlNode* TiXmlNode::InsertBeforeChild( TiXmlNode* beforeThis, const TiXmlNode& addThis )
{
	FAIL("TiXmlNode::InsertBeforeChild");
	return NULL;
}

const TiXmlElement* TiXmlNode::FirstChildElement()	const
{
	FAIL("TiXmlNode::FirstChildElement");
	return NULL;
}

const TiXmlElement* TiXmlNode::FirstChildElement( const char * _value ) const
{
	FAIL("TiXmlNode::FirstChildElement");
	return NULL;
}


const TiXmlDocument* TiXmlNode::GetDocument() const
{
	FAIL("TiXmlNode::GetDocument");
	return NULL;
}

bool TiXmlNode::RemoveChild( TiXmlNode* removeThis )
{
	FAIL("TiXmlNode::RemoveChild");
	return true;
}

const TiXmlElement* TiXmlNode::NextSiblingElement() const
{
	FAIL("TiXmlNode::NextSiblingElement");
	return NULL;
}

const TiXmlElement* TiXmlNode::NextSiblingElement( const char * ) const
{
	FAIL("TiXmlNode::NextSiblingElement");
	return NULL;
}


TiXmlNode* TiXmlNode::InsertEndChild( const TiXmlNode& addThis )
{
	FAIL("TiXmlNode::InsertEndChild");
	return NULL;
}

const TiXmlNode* TiXmlNode::IterateChildren( const TiXmlNode* previous ) const
{
	FAIL("TiXmlNode::IterateChildren");
	return NULL;
}

const TiXmlNode* TiXmlNode::IterateChildren( const char * value, const TiXmlNode* previous ) const
{
	FAIL("TiXmlNode::IterateChildren");
	return NULL;
}


TiXmlNode* TiXmlNode::LinkEndChild( TiXmlNode* addThis )
{
	FAIL("TiXmlNode::LinkEndChild");
	return NULL;
}

TiXmlDeclaration::TiXmlDeclaration(	const char* _version, const char* _encoding, const char* _standalone )
 : TiXmlNode(TINYXML_DECLARATION)
{
	FAIL("TiXmlDeclaration::TiXmlDeclaration");
}

TiXmlNode* TiXmlDeclaration::Clone() const
{
	FAIL("TiXmlDeclaration::Clone");
	return NULL;
}

void TiXmlDeclaration::Print( FILE* cfile, int depth, TIXML_STRING* str ) const
{
	FAIL("TiXmlDeclaration::Print");
}

const char* TiXmlDeclaration::Parse( const char* p, TiXmlParsingData* data, TiXmlEncoding encoding )
{
	FAIL("TiXmlDeclaration::Parse");
	return NULL;
}

bool TiXmlDeclaration::Accept( TiXmlVisitor* visitor ) const
{
	FAIL("TiXmlDeclaration::Accept");
	return true;
}

void TiXmlDeclaration::StreamIn( std::istream * in, TIXML_STRING * tag )
{
	FAIL("TiXmlDeclaration::StreamIn");
}

void TiXmlText::Print( FILE* cfile, int depth ) const
{
	FAIL("TiXmlText::Print");
}

const char* TiXmlText::Parse( const char* p, TiXmlParsingData* data, TiXmlEncoding encoding )
{
	FAIL("TiXmlText::Parse");
	return NULL;

}

TiXmlNode* TiXmlText::Clone() const
{
	FAIL("TiXmlText::Clone");
	return NULL;
}

bool TiXmlText::Accept( TiXmlVisitor* visitor ) const
{
	FAIL("TiXmlText::Accept");
	return true;
}

void TiXmlText::StreamIn( std::istream * in, TIXML_STRING * tag )
{
	FAIL("TiXmlText::StreamIn");
}

TiXmlDocument::TiXmlDocument()
 : TiXmlNode(TINYXML_DOCUMENT)
{
	FAIL("TiXmlDocument::TiXmlDocument");
}

bool TiXmlDocument::LoadFile( FILE*, TiXmlEncoding encoding)
{
	FAIL("TiXmlDocument::LoadFile");
	return true;
}

bool TiXmlDocument::SaveFile( FILE* ) const
{
	FAIL("TiXmlDocument::SaveFile");
	return true;
}

void TiXmlDocument::Print( FILE* cfile, int depth ) const
{
	FAIL("TiXmlDocument::Print");
}

const char* TiXmlDocument::Parse( const char* p, TiXmlParsingData* data, TiXmlEncoding encoding )
{
	FAIL("TiXmlDocument::Parse");
	return NULL;
}

TiXmlNode* TiXmlDocument::Clone() const
{
	FAIL("TiXmlDocument::Clone");
	return NULL;
}

bool TiXmlDocument::Accept( TiXmlVisitor* visitor ) const
{
	FAIL("TiXmlDocument::Accept");
	return true;
}

void TiXmlDocument::StreamIn( std::istream * in, TIXML_STRING * tag )
{
	FAIL("TiXmlDocument::StreamIn");
}

const char* TiXmlAttribute::Parse( const char* p, TiXmlParsingData* data, TiXmlEncoding encoding )
{
	FAIL("TiXmlAttribute::Parse");
	return NULL;
}

void TiXmlAttribute::Print( FILE* cfile, int depth, TIXML_STRING* str ) const
{
	FAIL("TiXmlAttribute::Print");
}

TiXmlElement::TiXmlElement (const char * in_value)
	:TiXmlNode(TINYXML_ELEMENT)
{
	FAIL("TiXmlElement::TiXmlElement");
}

TiXmlElement::~TiXmlElement()
{
	FAIL("TiXmlElement::~TiXmlElement");
}

const char* TiXmlElement::Attribute( const char* name ) const
{
	FAIL("TiXmlElement::Attribute");
	return NULL;
}

const char* TiXmlElement::Attribute( const char* name, int* i ) const
{
	FAIL("TiXmlElement::Attribute");
	return NULL;
}

void TiXmlElement::SetAttribute( const char* name, const char * _value )
{
	FAIL("TiXmlElement::SetAttribute");
}

void TiXmlElement::SetAttribute( const char * name, int value )
{
	FAIL("TiXmlElement::SetAttribute");
}

const char* TiXmlElement::GetText() const
{
	FAIL("TiXmlElement::GetText");
	return NULL;
}

void TiXmlElement::Print( FILE* cfile, int depth ) const
{
	FAIL("TiXmlElement::Print");
}

const char* TiXmlElement::Parse( const char* p, TiXmlParsingData* data, TiXmlEncoding encoding )
{
	FAIL("TiXmlElement::Parse");
	return NULL;
}

TiXmlNode* TiXmlElement::Clone() const
{
	FAIL("TiXmlElement::Clone");
	return NULL;
}

bool TiXmlElement::Accept( TiXmlVisitor* visitor ) const
{
	FAIL("TiXmlElement::Accept");
	return true;
}

void TiXmlElement::StreamIn( std::istream * in, TIXML_STRING * tag )
{
	FAIL("TiXmlElement::StreamIn");
}

bool TiXmlPrinter::VisitEnter( const TiXmlDocument& doc )
{
	FAIL("TiXmlPrinter::VisitEnter");
	return true;
}

bool TiXmlPrinter::VisitExit( const TiXmlDocument& doc )
{
	FAIL("TiXmlPrinter::VisitEnter");
	return true;
}

bool TiXmlPrinter::VisitEnter( const TiXmlElement& element, const TiXmlAttribute* firstAttribute )
{
	FAIL("TiXmlPrinter::VisitEnter");
	return true;
}

bool TiXmlPrinter::VisitExit( const TiXmlElement& element )
{
	FAIL("TiXmlPrinter::VisitExit");
	return true;
}

bool TiXmlPrinter::Visit( const TiXmlDeclaration& declaration )
{
	FAIL("TiXmlPrinter::Visit");
	return true;
}

bool TiXmlPrinter::Visit( const TiXmlText& text )
{
	FAIL("TiXmlPrinter::Visit");
	return true;
}

bool TiXmlPrinter::Visit( const TiXmlComment& comment )
{
	FAIL("TiXmlPrinter::Visit");
	return true;
}

bool TiXmlPrinter::Visit( const TiXmlUnknown& unknown )
{
	FAIL("TiXmlPrinter::Visit");
	return true;
}

TiXmlHandle TiXmlHandle::FirstChild() const
{
	FAIL("TiXmlHandle::FirstChild");
	return *this;
}

TiXmlHandle TiXmlHandle::FirstChildElement( const char * value ) const
{
	FAIL("TiXmlHandle::FirstChildElement");
	return *this;
}

