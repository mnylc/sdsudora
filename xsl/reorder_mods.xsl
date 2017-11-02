<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0"
  xmlns:mods="http://www.loc.gov/mods/v3"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">
  <xsl:output method="xml" version="1.0" encoding="UTF-8" indent="yes" media-type="text/xml"/>
  <xsl:strip-space elements="*"/>

  <!-- Copy the current element, some top levels such as container or as a utility during apply-templates. -->
  <xsl:template match="*">
    <xsl:copy>
      <xsl:copy-of select="@*"/>
      <xsl:apply-templates/>
    </xsl:copy>
  </xsl:template>

  <!-- Do shallow copy and recurse to get desired ordering on nested related items. -->
  <xsl:template match="mods:mods | mods:relatedItem">
    <xsl:copy>
      <xsl:copy-of select="@*"/>
      <xsl:apply-templates select="mods:titleInfo"/>
      <xsl:apply-templates select="mods:name"/>
      <xsl:apply-templates select="mods:typeOfResource"/>
      <xsl:apply-templates select="mods:genre"/>
      <xsl:apply-templates select="mods:originInfo"/>
      <xsl:apply-templates select="mods:language"/>
      <xsl:apply-templates select="mods:abstract"/>
      <xsl:apply-templates select="mods:subject"/>
      <xsl:apply-templates select="mods:relatedItem[@type='host']"/>
      <xsl:apply-templates select="mods:identifier"/>
      <xsl:apply-templates select="mods:location"/>
      <xsl:apply-templates select="mods:accessCondition"/>
      <xsl:apply-templates select="mods:extension"/>
      <xsl:apply-templates select="mods:recordInfo"/>
      <xsl:apply-templates select="*[not(
        self::mods:titleInfo |
        self::mods:name |
        self::mods:typeOfResource |
        self::mods:genre |
        self::mods:originInfo |
        self::mods:language |
        self::mods:abstract |
        self::mods:subject |
        self::mods:relatedItem[@type='host'] |
        self::mods:identifier |
        self::mods:location |
        self::mods:accessCondition |
        self::mods:extension |
        self::mods:recordInfo
      )]"/>
    </xsl:copy>
  </xsl:template>
</xsl:stylesheet>
