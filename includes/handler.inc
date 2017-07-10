<?php

/**
 * @file
 * Stores handler functionality for OAI Direct Link responses.
 */

/**
 * Wrapper around islandora_oai_object_response_xml().
 *
 * @param array $params
 *   @see islandora_oai_object_response_xml().
 *
 * @return null|string
 *   XML string output of the record depending on configuration, NULL if an
 *   error was encountered.
 */
function islandora_oai_download_link_object_response_xml($params) {
  module_load_include('inc', 'islandora_oai', 'includes/handler');
  $oai_output = islandora_oai_object_response_xml($params);
  if (is_null($oai_output)) {
    return NULL;
  }

  if (!islandora_oai_download_link_check_harvester_ip()) {
    return $oai_output;
  }

  switch ($params['metadata_prefix']) {
    case 'oai_dc':
      $oai_output = islandora_oai_download_modify_dc($params, $oai_output);
      break;
    case 'mods':
      $oai_output = islandora_oai_download_modify_mods($params, $oai_output);
      break;
  }

  // Handles responses for oai_etdms, which is returned unmodified,
  // can be added later. 
  return $oai_output;
}

/**
 * Form submit handler.
 */
function islandora_oai_download_link_handler_configuration_submit($form, $form_state) {
  variable_set('islandora_oai_download_link_cmodel_link_mappings', $form_state['values']['islandora_oai_download_links']['islandora_oai_download_link_cmodel_link_mappings']);
  variable_set('islandora_oai_download_link_allowed_ips', $form_state['values']['islandora_oai_download_links']['islandora_oai_download_link_allowed_ips']);
}

/**
 * Modifies the DC document by inserting a new dc:identifier.
 *
 * @param array $params
 *   @see islandora_oai_object_response_xml().
 * @param string $oai_dc
 *   The serialized DC document.
 *
 * @return string
 *   The modified DC document.
 */
function islandora_oai_download_modify_dc($params, $oai_dc) {
    $dom = new DOMDocument;
    $dom->loadXML($oai_dc);

    $dsid = islandora_oai_download_get_dsid($params['pid']);

    global $base_url;
    $download_url = $base_url . '/islandora/object/' . $params['pid'] . '/datastream/' . $dsid . '/download';

    $new_identifier = $dom->createElementNS('http://purl.org/dc/elements/1.1/', 'dc:identifier', $download_url);
    $xpath = new DomXPath($dom);
    $xpath->registerNamespace('dc', 'http://purl.org/dc/elements/1.1/');
    $xpath_query = '//dc:identifier';
    $identifiers = $xpath->evaluate($xpath_query);

    if ($identifiers->length) {
      // If some dc:identifiers already exist, we want our new dc:identifier
      // to be the first one.
      $dom->documentElement->insertBefore($new_identifier, $identifiers->item(0));
      $modified_oai_dc = $dom->saveXML($dom->documentElement);
    }
    else {
      // If none exist, add the new one to the end of the DC document.
      $dom->documentElement->appendChild($new_identifier);
    }

    return $modified_oai_dc;
}

/**
 * Modifies the MODS document.
 *
 * @param array $params
 *   @see islandora_oai_object_response_xml().
 * @param string $mods
 *   The serialized MODS document.
 *
 * @return string
 *   The modified MODS document.
 */
function islandora_oai_download_modify_mods($params, $mods) {
  // For now, return the XML unmodified.
  return $mods;
}

/**
 * Gets the datastream ID to insert into the download URL.
 *
 * @param string $pid
 *   The current object's PID.
 *
 * @return string|bool
 *   The DSID, FALSE if none match.
 */
function islandora_oai_download_get_dsid($pid) {
  $object = islandora_object_load($pid);
  $object_cmodels = $object->models;
  $map = variable_get('islandora_oai_download_link_cmodel_link_mappings', ISLANDORA_OAI_DOWLOAD_LINK_DEFAULT_CMODEL_LINK_MAPPINGS);
  $mappings = preg_split("/\\r\\n|\\r|\\n/", $map);
  foreach ($mappings as $mapping) {
    list($cmodel, $dsid) = explode('=', $mapping);
    $cmodel = trim($cmodel);
    $dsid = trim($dsid);
    $dsids[$cmodel] = $dsid;
  }

  foreach ($object_cmodels as $object_cmodel) {
    if (in_array($object_cmodel, array_keys($dsids))) {
      return $dsids[$cmodel];
    }
  }
}

/**
 * Checks the OAI harvester's IP address against the configured whitelist.
 *
 * @return bool
 *   TRUE if the harvester is in the whitelist, FALSE if not.
 */
function islandora_oai_download_link_check_harvester_ip() {
  // @todo: Complete this.
  $whitelist = variable_get('islandora_oai_download_link_allowed_ips', '');
  return TRUE;
}
