<?php

class TfaRecoveryCode extends TfaBasePlugin implements TfaValidationPluginInterface {

  public function __construct(array $context = array()) {
    parent::__construct($context);
    $this->code = 'FAILSAFE'; // @todo
  }

  public function getForm($form) {
    $form['recover'] = array(
      '#type' => 'textfield',
      '#title' => t('Enter one of your recovery codes'),
      '#required' => TRUE,
      '#description' => t('These recovery codes were generated when you first setup TFA.'),
    );
    $form['actions']['login'] = array(
      '#type' => 'submit',
      '#value' => t('Submit'),
    );
    return $form;
  }

  public function validateForm(&$form_state) {
    if (!parent::validate($form_state['values']['recover'])) {
      $this->errorMessages['code'] = t('Invalid recovery code.');
      return FALSE;
    }
    else {
      return TRUE;
    }
  }
}
