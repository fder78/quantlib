#pragma once

enum E_ProductType
{
	PT_CLNSingle,
	PT_CLNNtD,
	PT_CDSSingle,
	PT_CDSNtD,
	PT_RANote,
	PT_RASwap,
};

class TiXmlElement;

class IProductParam
{
public:
	IProductParam();
	void SetData( const TiXmlElement* record );
	virtual void Calculate() = 0;

	static boost::shared_ptr<IProductParam> Create( const TiXmlElement* record );
	boost::shared_ptr<const TiXmlElement> GetFinalResult() const { return m_result; }

	std::string GetProductName() const { return m_productName; }

protected:
	virtual void SetDataImpl( const TiXmlElement* record ) = 0;

	boost::shared_ptr<TiXmlElement> GetResultObject() { return m_result; }

private:
	std::string m_productName;

	boost::shared_ptr<TiXmlElement> m_record;
	boost::shared_ptr<TiXmlElement> m_result;
};
