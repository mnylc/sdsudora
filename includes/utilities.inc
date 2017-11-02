<?php

/**
 * @file
 * Utility functions for the module.
 */

/**
 * This function will extract object information from a mets.xml file.
 */
function sdsudora_extract_data_from_mets($path) {
  $info = array('children' => array());

  $dom = new DOMDocument();
  $dom->load($path);
  $xpath = new DOMXpath($dom);
  $xpath->registerNameSpace('mets', 'http://www.loc.gov/METS/');
  $xpath->registerNameSpace('xlink', 'http://www.w3.org/1999/xlink');
  $xpath->registerNameSpace('mods', 'http://www.loc.gov/mods/v3');
  $xpath->registerNameSpace('dim', 'http://www.dspace.org/xmlns/dspace/dim');

  // Resolve the hierarchy.
  $children_elems = $xpath->query('/mets:mets/mets:structMap/mets:div/mets:div/mets:mptr[2]/@xlink:href');
  foreach ($children_elems as $child) {
    // Stripping the extension.
    $info['children'][] = substr($child->value, 0, -4);
  }
  $parent_elems = $xpath->query('/mets:mets/mets:structMap/mets:div/mets:mptr/@xlink:href');
  foreach ($parent_elems as $parent) {
    $info['parents'][] = $parent->value;
  }

  // @XXX: This is suspicious and may need changing in the future. This handle
  // extraction was based on a single export and we have no confidence that
  // this will be standard in DSpace exports in the future.
  $object_id = $xpath->query('/mets:mets/@OBJID')->item(0)->value;
  $info['handle'] = substr($object_id, strpos($object_id, ':') + 1);

  // Extract label.
  $label_nodes = $xpath->query('/mets:mets/mets:dmdSec/mets:mdWrap/mets:xmlData/mods:mods/mods:titleInfo/mods:title');
  if ($label_nodes->length) {
    $info['label'] = $label_nodes->item(0)->textContent;
  }

  // Extract type.
  $info['type'] = $xpath->query('/mets:mets/@TYPE')->item(0)->value;

  // Extract OBJ.
  // @XXX: This may need iteration when we have more DSpace exports.
  $obj_elem = $xpath->query('/mets:mets/mets:fileSec/mets:fileGrp[@USE="ORIGINAL"]/mets:file')->item(0);
  if ($obj_elem) {
    $info['OBJ'] = array(
      'checksum' => $obj_elem->getAttribute('CHECKSUM'),
      'checksum_type' => $obj_elem->getAttribute('CHECKSUMTYPE'),
      'mimetype' => $obj_elem->getAttribute('MIMETYPE'),
      'path' => dirname($path) . DIRECTORY_SEPARATOR . $xpath->query('mets:FLocat/@xlink:href', $obj_elem)->item(0)->value,
    );
  }

  $info['metadata'] = $dom->saveXML($xpath->query('/mets:mets/mets:dmdSec/mets:mdWrap/mets:xmlData/dim:dim')->item(0));

  return $info;
}

/**
 * Ingest the metadata.
 * XXX: Could future-proof by passing in more params.
 */
function sdsudora_add_metadata($object, $mods_xml) {
  if ($mods_xml) {
    $mods_datastream = $object->constructDatastream('MODS', 'M');
    $mods_datastream->mimetype = 'application/xml';
    $mods_datastream->label = "MODS Datastream";
    $mods_datastream->content = $mods_xml;
    $object->ingestDatastream($mods_datastream);
    return TRUE;
  }
  return FALSE;
}

/**
 * Generate MODS content.
 */
function sdsudora_generate_mods($object_info) {
  // Mappings for DIM to MODS.
  $terms_map = array(
    'dc.contributor.author' => '<mods:name><mods:namePart /><mods:role><mods:roleTerm authority="marcrelator" type="text">author</mods:roleTerm></mods:role></mods:name>',
    'dc.contributor.committeemember' => '<mods:name type="personal"><mods:displayForm /><mods:role><mods:roleTerm>Committee Member</mods:roleTerm></mods:role></mods:name>',
    'dc.contributor.committeeMember' => '<mods:name type="personal"><mods:displayForm /><mods:role><mods:roleTerm>Committee Member</mods:roleTerm></mods:role></mods:name>',
    'dc.contributor.advisor' => '<mods:name><mods:displayForm /><mods:role><mods:roleTerm authority="marcrelator" type="text">Thesis advisor</mods:roleTerm></mods:role></mods:name>',
    'dc.contributor.department' => '<mods:name type="corporate"><mods:displayForm /><mods:role><mods:roleTerm type="text">department</mods:roleTerm></mods:role></mods:name>',
    'dc.contributor.editor' => '<mods:name><mods:displayForm /><mods:role><mods:roleTerm authority="marcrelator" type="text">editor</mods:roleTerm></mods:role></mods:name>',
    'dc.coverage.spatial' => '<mods:subject><mods:hierarchicalgeographic><mods:area /></mods:hierarchicalgeographic></mods:subject>',
    'dc.coverage.box' => '<mods:subject><mods:cartographics><mods:coordinates /></mods:cartographics></mods:subject>',
    'dc.coverage.point' => '<mods:subject><mods:cartographics><mods:coordinates /></mods:cartographics></mods:subject>',
    'dc.date.updated' => '<mods:originInfo><mods:dateModified /></mods:originInfo>',
    'dc.date.embargountil' => '<mods:accessCondition type="restriction on access" />',
    'dc.date.semester' => '<mods:originInfo><mods:dateValid /></mods:originInfo>',
    'dc.date.available' => '<mods:originInfo><mods:dateCaptured /></mods:originInfo>',
    'dc.date.issued' => '<mods:originInfo><mods:dateIssued /></mods:originInfo>',
    'dc.identifier.other' => '<mods:identifier type="local" />',
    'dc.identifier.uri' => '<mods:identifier type="uri" />',
    'dc.description' => '<mods:abstract type="detail" />',
    'dc.description.abstract' => '<mods:abstract />',
    'dc.description.degree' => '<mods:extension><etd:degree><etd:name /></etd:degree></mods:extension>',
    'dc.description.discipline' => '<mods:extension><etd:degree><etd:discipline /></etd:degree></mods:extension>',
    'dc.description.sponsorship' => '<mods:note type="sponsorship" />',
    'dc.description.embargoterms' => '<mods:accessCondition type="embargo terms" />',
    'dc.description.statementofresponsibility' => '<mods:name><mods:namePart /><mods:role><mods:roleTerm authority="marcrelator" type="text">teacher</mods:roleTerm></mods:role></mods:name>',
    'dc.format.extent' => '<mods:physicalDescription><mods:extent /></mods:physicalDescription>',
    'dc.language' => '<mods:language><mods:languageTerm type="text" /></mods:language>',
    'dc.language.rfc3066' => '<mods:language><mods:languageTerm authority="rfc3066" type="code" /></mods:language>',
    'dc.language.iso' => '<mods:language><mods:languageTerm authority="iso639-2b" type="code" /></mods:language>',
    'dc.provenance' => '<mods:recordInfo><mods:recordOrigin /></mods:recordInfo>',
    'dc.publisher' => '<mods:extension><etd:degree><etd:grantor /></etd:degree></mods:extension>',
    'dc.relation.hasversion' => '<mods:relatedItem><mods:identifier type="local" /><mods:physicalDescription><mods:form>microfiche</mods:form></mods:physicalDescription></mods:relatedItem>',
    'dc.relation.ispartofseries' => '<mods:relatedItem type="series"><mods:titleInfo><mods:partNumber /></mods:titleInfo></mods:relatedItem>',
    'dc.rights' => '<mods:accessCondition type="use and reproduction" />',
    'dc.subject' => '<mods:subject><mods:topic /></mods:subject>',
    'dc.subject.lcc' => '<mods:classification authority="lcc" />',
    'dc.title' => '<mods:titleInfo><mods:title /></mods:titleInfo>',
    'dc.title.alternative' => '<mods:titleInfo type="alternative"><mods:title /></mods:titleInfo>',
    'dc.type' => '<mods:genre />',
    'dc.type.genre' => '<mods:genre />',
  );

  $dom = new DOMDocument();
  $dom->formatOutput = TRUE;
  $root = $dom->createElementNS('http://www.loc.gov/mods/v3', 'mods:mods');
  $root->setAttributeNS('http://www.w3.org/2000/xmlns/', 'xmlns:etd', 'http://www.ndltd.org/standards/metadata/etdms/1.0');

  $dom->appendChild($root);
  $old_doc = new DOMDocument();
  $old_doc->loadXML($object_info);
  // Here we are relying on DSpace's configuration conventions.
  foreach ($old_doc->documentElement->childNodes as $child) {
    if ($child->nodeType !== XML_ELEMENT_NODE) {
      continue;
    }
    $key_parts = array(
      $child->getAttribute('mdschema'),
      $child->getAttribute('element'),
    );
    if ($child->hasAttribute('qualifier')) {
      $key_parts[] = $child->getAttribute('qualifier');
    }
    $key = implode('.', $key_parts);

    // Flag error if there is no entry in the termsMap.
    if (!isset($terms_map[$key])) {
      continue;
    }
    $tmp_doc = new DOMDocument();
    @$tmp_doc->loadXML($terms_map[$key]);
    $tmp_root = $tmp_doc->documentElement;
    $element = $tmp_root;
    // Traverse down the tree until we get to the bottom-most node.
    while ($element->hasChildNodes()) {
      if ($element->childNodes->length == 1) {
        $element = $element->firstChild;
      }
      elseif ($element->childNodes->length > 1) {
        // Search children for first element node.
        $children = $element->childNodes;
        foreach ($children as $child_node) {
          // In the case of multiple siblings, just find the first element type.
          // XXX: We could add logic in the future to specify which sibling we
          // insert data into.
          if ($child_node->nodeType == XML_ELEMENT_NODE) {
            $element = $child_node;
            break;
          }
        }
      }

    }

    $element->appendChild($tmp_doc->createTextNode($child->textContent));
    $root->appendChild($dom->importNode($tmp_root, TRUE));
  }
  // XXX: Workaround: XSLT doesn't seem to like working on $dom directly
  // saving to a new DOM works.
  $newDom = new DOMDocument();
  $newDom->loadXML($dom->saveXML());
  return sdsudora_reorder_mods($newDom);
}

/**
 * Reorder MODS structure.
 */
function sdsudora_reorder_mods($dom) {
  $xslt = new XSLTProcessor();
  $module_path = drupal_get_path('module', 'sdsudora');
  $xsl = new DOMDocument();
  $xsl->load("$module_path/xsl/reorder_mods.xsl");
  $xslt->importStylesheet($xsl);
  return $xslt->transformToXml($dom);
}
