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
  variable_set('islandora_oai_download_link_mods_fragment', $form_state['values']['islandora_oai_download_links']['islandora_oai_download_link_mods_fragment']);
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
  $dom = new DOMDocument;
  $dom->preserveWhiteSpace = FALSE;
  $dom->formatOutput = TRUE;
  $dom->loadXML($mods);

  $dsid = islandora_oai_download_get_dsid($params['pid']);
  global $base_url;
  $download_url = $base_url . '/islandora/object/' . $params['pid'] . '/datastream/' . $dsid . '/download';

  $fragment_template = variable_get('islandora_oai_download_link_mods_fragment', '<location><url access="download">%url%</url></location>');
  if (!strlen($fragment_template)) {
    return $mods;
  }

  $fragment_populated = preg_replace('/%url%/', $download_url, $fragment_template);
  $fragment = $dom->createDocumentFragment();
  $fragment->appendXML($fragment_populated);
  $dom->documentElement->appendChild($fragment);

  $modified_mods = $dom->saveXML($dom->documentElement);

  return $modified_mods;
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
      return $dsids[$object_cmodel];
    }
  }
}

/**
 * Checks the OAI harvester's IP address against the configured whitelist.
 *
 * CIDR notation check based on example provided by claudiu at cnixs dot com
 * at http://php.net/manual/en/ref.network.php.
 *
 * @return bool
 *   TRUE if the harvester is in the whitelist or if there is no whitelist,
 *   FALSE if not.
 */
function islandora_oai_download_link_check_harvester_ip() {
  $client_address = ip2long(ip_address());

  $ip_ranges = variable_get('islandora_oai_download_link_allowed_ips', '');
  if (!strlen($ip_ranges)) {
    return TRUE;
  }

  // If there is a whitelist, check the client IP address against
  // the configured CIDR ranges.
  $is_allowed = FALSE;
  $cidrs = preg_split("/\\r\\n|\\r|\\n/", $ip_ranges);
  foreach ($cidrs as $cidr) {
    list ($net, $mask) = split ("/", $cidr);
    $ip_net = ip2long($net);
    $ip_mask = ~((1 << (32 - $mask)) - 1);
    $ip_ip = ip2long($client_address);
    $ip_ip_net = $ip_ip & $ip_mask;
    if ($ip_ip_net == $ip_net) {
      $is_allowed = TRUE;
    }
  }

  return $is_allowed;
}
