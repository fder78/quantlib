#pragma once

enum E_ProductType
{
	PT_CLNSingle,
	PT_CLNNtD,
	PT_CDSSingle,
	PT_CDSNtD,
	PT_RANote,
	PT_RASwap,
	PT_RAFDMNote,
	PT_RAFDMSwap,
	PT_RANoteKAP,
	PT_RASwapKAP,
	PT_RANoteCMS,
	PT_RASwapCMS,
	PT_RANoteSingleTree,
	PT_RASwapSingleTree,
	PT_RASpreadNote,
	PT_RASpreadSwap,
	PT_VanillaSwap,
	PT_FixedRateBond,
	PT_PSSwap,
	PT_PSNote,
	PT_CapFloor,
	PT_Swaption,
	PT_FXDigital,
};

class TiXmlElement;

class IProductParam
{
public:
	IProductParam();
	void SetData( const TiXmlElement* record );
	virtual void Calculate() = 0;
	virtual void FetchResult() { }

	static boost::shared_ptr<IProductParam> Create( const TiXmlElement* record );
	boost::shared_ptr<const TiXmlElement> GetFinalResult() const { return m_result; }

	std::string GetProductName() const { return m_productName; }
	std::string GetPrductType() const { return m_type; }

	virtual void ParseResult( const TiXmlElement& result );

protected:
	virtual void SetDataImpl( TiXmlElement* record ) = 0;

	boost::shared_ptr<TiXmlElement> GetResultObject() { return m_result; }

	boost::shared_ptr<TiXmlElement> GetRecord() const { return m_record; }

private:
	std::string m_productName;
	std::string m_type;

	std::string m_strTraderID;
	std::string m_strBook;

	boost::shared_ptr<TiXmlElement> m_record;
	boost::shared_ptr<TiXmlElement> m_result;
};
