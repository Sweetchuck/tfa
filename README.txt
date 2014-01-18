Two-factor Authentication for Drupal


### How to generate and send a code to a user

Create a plugin that implements the TfaSendPluginInterface interface and
implement the 'begin' method. It will be invoked whenever a user is beginning
the TFA process. Your plugin can then generate a code (or use the base plugin
'generate' method) and send that code to the user.

Your plugin should either store the generated code itself (such as in the Drupal
database) or set it to the plugin context so it can be made available for
validation. See the section on Plugin Context for more.
