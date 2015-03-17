/**
 * @file
 * Behaviors of TFA admin settings form.
 */

(function ($) {
  'use strict';

  Drupal.behaviors.tfaAdminSettings = function (context, settings) {
    var $form = $('form.tfa-admin-settings-form');

    $('input[name="tfa_enabled"]:not(.tfa-admin-settings-processed)', $form)
      .addClass('tfa-admin-settings-processed')
      .change(Drupal.tfa.adminSettingsEnabledOnChange)
      .trigger('change');

    $(':input[name="tfa_validate"]:not(.tfa-admin-settings-processed)', $form)
      .addClass('tfa-admin-settings-processed')
      .change(Drupal.tfa.adminSettingsDefaultValidatorOnChange)
      .trigger('change');

    $(':input.tfa-fallback-plugin-enabler', $form)
      .addClass('tfa-admin-settings-processed')
      .change(Drupal.tfa.adminSettingsFallbackPluginEnableOnChange)
      .trigger('change');
  };

  Drupal.tfa = Drupal.tfa || {};

  Drupal.tfa.adminSettingsEnabledOnChange = function () {
    if (this.checked) {
      $('.tfa-options-wrapper', $(this).parents('form')).show();
    }
    else {
      $('.tfa-options-wrapper', $(this).parents('form')).hide();
    }
  };

  Drupal.tfa.adminSettingsDefaultValidatorOnChange = function () {
    var
      $form = $(this).parents('form'),
      pluginName = $(this).val();

    $('.tfa-options-fallback-plugin', $form).show();
    $('.tfa-options-fallback-plugin-' + pluginName, $form).hide();

    Drupal.tfa.adminSettingsUpdatePluginSettingsVisibility($form);
  };

  Drupal.tfa.adminSettingsFallbackPluginEnableOnChange = function () {
    Drupal.tfa.adminSettingsUpdatePluginSettingsVisibility($(this).parents('form'));
  };

  Drupal.tfa.adminSettingsUpdatePluginSettingsVisibility = function ($form) {
    var defaultValidator = $(':input[name="tfa_validate"]', $form).val();

    $('.tfa-options-settings-plugin', $form).hide();
    $('input.tfa-fallback-plugin-enabler:checked', $form).each(function () {
      var pluginName = $(this).attr('name').replace(/(^tfa_fallback\[)|(\]\[enable\]$)/g, '');

      $('.tfa-options-settings-plugin-' + pluginName, $form).show();
    });

    $('.tfa-options-settings-plugin-' + defaultValidator, $form).show();
  };

}(jQuery));
