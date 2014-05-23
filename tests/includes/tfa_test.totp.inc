<?php

class TfaTestTotp extends TfaBasePlugin implements TfaValidationPluginInterface {

  public function ready() {
    return variable_get('tfa_test_is_ready', TRUE);
  }

  public function getForm(array $form, array &$form_state) {
    $form['code'] = array(
      '#type' => 'textfield',
      '#title' => t('TOTP code'),
      '#required' => TRUE,
    );
    $form['login'] = array(
      '#type' => 'submit',
      '#value' => t('Submit'),
    );

    return $form;
  }

  public function validateForm(array $form, array &$form_state) {
      $this->code = variable_get('tfa_totp', 123456);
      if (!parent::validate($form_state['values']['code'])) {
        $this->errorMessages['code'] = t('Invalid TOTP code');
        return FALSE;
      }
      else {
        return TRUE;
      }
    }
}
