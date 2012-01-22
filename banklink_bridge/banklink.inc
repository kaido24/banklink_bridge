<?php
// create icon html for other modules to use..
// Usaqe put icon into images folder and name this to bank name. After this can be called from other module with current function
function banklink_get_icon($bank_name) {
			$src = base_path() . "sites/all/libraries/banklink_bridge/images/$bank_name.gif";
			$name = ucfirst($bank_name);
			return <<<html
<img style="vertical-align: middle" src="$src" alt="$name" />
html;

}
function banklink_create_signature($VK, $merchant_private_key, $passphrase = '') {
  $data = banklink_compose_data($VK);
  $pkeyid = openssl_get_privatekey($merchant_private_key, $passphrase);

  openssl_sign($data, $signature, $pkeyid);
  $VK_MAC = base64_encode($signature);

  openssl_free_key($pkeyid);

  return $VK_MAC;
}

function banklink_create_nordea_signature($VK, $settings) {
  if(isset($settings['merchant_mac'])) {
  return strtoupper(call_user_func($settings['hash'], "{$VK['SOLOPMT_VERSION']}&{$VK['SOLOPMT_STAMP']}&{$VK['SOLOPMT_RCV_ID']}&{$VK['SOLOPMT_AMOUNT']}&{$VK['SOLOPMT_REF']}&{$VK['SOLOPMT_DATE']}&{$VK['SOLOPMT_CUR']}&{$settings['merchant_mac']}&"));
  }
}

function banklink_create_nordea_return_signature($VK, $settings) {
  return strtoupper(call_user_func($settings['hash'], "{$VK['SOLOPMT_RETURN_VERSION']}&{$VK['SOLOPMT_RETURN_STAMP']}&{$VK['SOLOPMT_RETURN_REF']}&{$VK['SOLOPMT_RETURN_PAID']}&{$settings['merchant_mac']}&"));
}

function banklink_verify_signature($VK_a, $settings) {

  $VK_MAC = $VK_a['VK_MAC'];
  $signature = base64_decode($VK_MAC);

  switch( $VK_a['VK_SERVICE'] ) {
  
    case '1101':
      $data =
      banklink_str_pad($VK_a['VK_SERVICE']) .
      banklink_str_pad($VK_a['VK_VERSION']) .
      banklink_str_pad($VK_a['VK_SND_ID'])  .
      banklink_str_pad($VK_a['VK_REC_ID']) .
      banklink_str_pad($VK_a['VK_STAMP']) .
      banklink_str_pad($VK_a['VK_T_NO']) .
      banklink_str_pad($VK_a['VK_AMOUNT']) .
      banklink_str_pad($VK_a['VK_CURR']) .
      banklink_str_pad($VK_a['VK_REC_ACC']) .
      banklink_str_pad($VK_a['VK_REC_NAME']) .
      banklink_str_pad($VK_a['VK_SND_ACC']) .
      banklink_str_pad($VK_a['VK_SND_NAME']) .
      banklink_str_pad($VK_a['VK_REF']) .
      banklink_str_pad($VK_a['VK_MSG']) .
      banklink_str_pad($VK_a['VK_T_DATE']);
      break;
    case '1901':
      $data =
      banklink_str_pad($VK_a['VK_SERVICE']) .
      banklink_str_pad($VK_a['VK_VERSION']) .
      banklink_str_pad($VK_a['VK_SND_ID']) .
      banklink_str_pad($VK_a['VK_REC_ID']) .
      banklink_str_pad($VK_a['VK_STAMP']) .
      banklink_str_pad($VK_a['VK_REF']) .
      banklink_str_pad($VK_a['VK_MSG']);
      break;
  }

  $pubkey = openssl_get_publickey($settings['bank_public_key']);
  $out = @openssl_verify($data, $signature, $pubkey);
  @openssl_free_key($pubkey);

  return $out;
}

function banklink_verify_nordea_signature($VK_a, $settings) {
  return $VK_a['SOLOPMT_RETURN_MAC'] == banklink_create_nordea_return_signature($VK_a, $settings);
}

/**
 * Teeb numbrist viitenumbri lisades sinna vajaliku kontrollarvu
 */
function banklink_get_reference_number($str)
{
  $weights = array(7, 3, 1, 7, 3, 1, 7, 3, 1, 7, 3, 1, 7, 3, 1, 7, 3, 1, 7, 3);
  $str_a = preg_split("//", $str, -1, PREG_SPLIT_NO_EMPTY);
  $sum = 0;

  $weights = array_reverse(array_slice($weights, 0, count($str_a)));

  foreach ($str_a AS $index => $num) {
    $add = $num * $weights[$index];
    $sum += $add;
  }

  if (($sum % 10) != 0) {
    $jrk = (10 - ($sum%10));
  } else {
    $jrk = 0;
  }
  return "$str$jrk";
}

function banklink_compose_data(&$VK_a) {
  $data ='';
  foreach($VK_a AS $data_bit) {
    $data .= banklink_str_pad($data_bit);
  }
  return $data;
}

function banklink_str_pad($str = "") {
  return str_pad(strlen($str), 3, "0", STR_PAD_LEFT) . $str;
}

function banklink_expand_payment_form(&$form, $settings, $payment) {
  $VK = array(
    'VK_SERVICE' => 1002,
    'VK_VERSION' => '008',
    'VK_SND_ID' => $settings['merchant_id'],
    'VK_STAMP' => $payment['order_id'],
    'VK_AMOUNT' => number_format($payment['amount'], 2, '.', ''),
    'VK_CURR' => $payment['currency'],
    'VK_REF' => banklink_get_reference_number($payment['order_id']),
    'VK_MSG' => $payment['message'],
  );

  $form['#action'] = $settings['banklink_url'];

  $form['VK_SERVICE'] = array(
    '#type' => 'hidden',
    '#value' => $VK['VK_SERVICE'],
  );
  $form['VK_VERSION'] = array(
    '#type' => 'hidden',
    '#value' => $VK['VK_VERSION'],
  );
  $form['VK_SND_ID'] = array(
    '#type' => 'hidden',
    '#value' => $VK['VK_SND_ID'],
  );
  $form['VK_STAMP'] = array(
    '#type' => 'hidden',
    '#value' => $VK['VK_STAMP'],
  );
  $form['VK_AMOUNT'] = array(
    '#type' => 'hidden',
    '#value' => $VK['VK_AMOUNT'],
  );
  $form['VK_CURR'] = array(
    '#type' => 'hidden',
    '#value' => $VK['VK_CURR'],
  );
  $form['VK_REF'] = array(
    '#type' => 'hidden',
    '#value' => $VK['VK_REF'],
  );
  $form['VK_MSG'] = array(
    '#type' => 'hidden',
    '#value' => $VK['VK_MSG'],
  );
  $form['VK_MAC'] = array(
    '#type' => 'hidden',
    '#value' => banklink_create_signature($VK, $settings['merchant_private_key'], $settings['merchant_private_key_passphrase']),
  );
  $form['VK_LANG'] = array(
    '#type' => 'hidden',
    '#value' => 'EST',
  );
  $form['VK_RETURN'] = array(
    '#type' => 'hidden',
    '#value' => $settings['return'],
  );
  $form['VK_CANCEL'] = array(
    '#type' => 'hidden',
    '#value' => $settings['cancel_return'],
  );
}


//nordea payment
function banklink_expand_nordea_payment_form(&$form, $settings, $payment) {
  $VK = array(
    'SOLOPMT_VERSION' => '0003',
    'SOLOPMT_STAMP' => $payment['order_id'],
    'SOLOPMT_RCV_ID' => $settings['merchant_id'],
    // 3 = English 4 = Estonian 6 = Latvian 7 = Lithuanian
    'SOLOPMT_LANGUAGE' => 4,
    'SOLOPMT_AMOUNT' => number_format($payment['amount'], 2, '.', ''),
    'SOLOPMT_REF' => banklink_get_reference_number($payment['order_id']),
    'SOLOPMT_DATE' => 'EXPRESS',
    'SOLOPMT_MSG' => $payment['message'],
    'SOLOPMT_KEYVERS' => '0001',
    'SOLOPMT_CUR' => $payment['currency'],
    'SOLOPMT_CONFIRM' => 'YES',
  );

  $form['#action'] = $settings['banklink_url'];

  $form['SOLOPMT_VERSION'] = array(
    '#type' => 'hidden',
    '#value' => $VK['SOLOPMT_VERSION'],
  );
  $form['SOLOPMT_STAMP'] = array(
    '#type' => 'hidden',
    '#value' => $VK['SOLOPMT_STAMP'],
  );
  $form['SOLOPMT_RCV_ID'] = array(
    '#type' => 'hidden',
    '#value' => $VK['SOLOPMT_RCV_ID'],
  );
  $form['SOLOPMT_LANGUAGE'] = array(
    '#type' => 'hidden',
    '#value' => $VK['SOLOPMT_LANGUAGE'],
  );
  $form['SOLOPMT_AMOUNT'] = array(
    '#type' => 'hidden',
    '#value' => $VK['SOLOPMT_AMOUNT'],
  );
  $form['SOLOPMT_REF'] = array(
    '#type' => 'hidden',
    '#value' => $VK['SOLOPMT_REF'],
  );
  $form['SOLOPMT_DATE'] = array(
    '#type' => 'hidden',
    '#value' => $VK['SOLOPMT_DATE'],
  );
  $form['SOLOPMT_MSG'] = array(
    '#type' => 'hidden',
    '#value' => $VK['SOLOPMT_MSG'],
  );
  $form['SOLOPMT_RETURN'] = array(
    '#type' => 'hidden',
    '#value' =>  $settings['return'],
  );
  $form['SOLOPMT_CANCEL'] = array(
    '#type' => 'hidden',
    '#value' => $settings['cancel_return'],
  );
  $form['SOLOPMT_REJECT'] = array(
    '#type' => 'hidden',
    '#value' => $settings['cancel_return'],
  );
  $form['SOLOPMT_MAC'] = array(
    '#type' => 'hidden',
    '#value' => banklink_create_nordea_signature($VK, $settings),
  );
  $form['SOLOPMT_CONFIRM'] = array(
    '#type' => 'hidden',
    '#value' => $VK['SOLOPMT_CONFIRM'],
  );
  $form['SOLOPMT_KEYVERS'] = array(
    '#type' => 'hidden',
    '#value' => $VK['SOLOPMT_KEYVERS'],
  );
  $form['SOLOPMT_CUR'] = array(
    '#type' => 'hidden',
    '#value' => $VK['SOLOPMT_CUR'],
  );
}
