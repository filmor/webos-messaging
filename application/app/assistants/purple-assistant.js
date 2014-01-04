
function PurpleAssistant(params) {
    this.template = params.initialTemplate || params.template;

    this.username = params.username || "";
    this.password = params.password || "";
    this.prpl = this.template.prpl;
    this.prefs = this.template.preferences || {};
};

var _ValidatorAddress = "palm://org.webosinternals.purple.validator/";

PurpleAssistant.prototype.setup = function() {
    this.controller.get('header').innerHTML = this.template.loc_name;

    // Username
    this.usernameModel = {
        value: this.username,
        disabled: false
    };

    this.controller.get('loc_usernameLabel').innerHTML =
        this.template.loc_usernameLabel || "Username";

    this.controller.setupWidget('username',
        {
            hintText: this.template.loc_usernameExplainLabel || "",
            autoFocus: true,
            textCase: Mojo.Widget.steModeLowerCase
        },
        this.usernameModel
    );

    // Password
    this.passwordModel = {
        value: this.password,
        disabled: false
    };

    this.controller.get('loc_passwordLabel').innerHTML =
        this.template.loc_passwordLabel || "Password";

    this.controller.setupWidget('password',
        {
            textFieldName: "loc_passwordLabel",
            hintText: this.template.loc_passwordExplainLabel || ""
        },
        this.passwordModel
    );

    // Setup CreateAccount widget
    this.controller.setupWidget('CreateAccountButton',
        {
            type: Mojo.Widget.activityButton
        },
        {
            buttonClass: 'primary',
            buttonLabel: 'Sign In',
            disabled: false
        }
    );

    // Setup OptionsList
    this.optionsModel = {
        items: []
    };

    this.controller.setupWidget("OptionsList", {
            itemTemplate: "../templates/options-item",
            swipeToDelete: false,
            autoconfirmDelete: false,
            reorderable: false
        },
        this.optionsModel);

    // Call getOptions
    this.controller.serviceRequest(_ValidatorAddress + "getOptions", {
        parameters: {
            prpl: this.template.prpl,
            locale: Mojo.Locale.getCurrentLocale()
        },
        onSuccess: this.optionsSuccess.bind(this),
        onFailure: function(response) {
            Mojo.Log.error("Couldn't get options: " + JSON.stringify(response));
        }
    });
};

PurpleAssistant.prototype.optionsSuccess = function(response) {
    Mojo.Log.info("optionsSuccess: " + JSON.stringify(response));
    var stripped_options = {};
    for (var name in response.options)
        if (!(name in this.prefs))
            stripped_options[name] = response.options[name];
    
    this.createOptionsWidget(stripped_options);

    this.controller.listen(this.controller.get('CreateAccountButton'),
            Mojo.Event.tap,
            this.createAccount.bindAsEventListener(this)
        );
};

PurpleAssistant.prototype.createAccount = function(ev) {
    Mojo.Log.info("Creating account");
    if (this.usernameModel.value === "")
    {
        Mojo.Log.error("No username entered");
        Mojo.Controller.errorDialog("Please enter a valid username");
        this.enableControls();
        return;
    }

    this.disableControls();

    this.controller.serviceRequest(_ValidatorAddress + "validateAccount", {
        parameters: {
            "username": this.usernameModel.value,
            "password": this.passwordModel.value,
            "prpl": this.template.prpl,
            "templateId": this.template.templateId,
            "config": this.prefs
        },
        onFailure: this.eventFail.bind(this),
        onError: this.eventFail.bind(this)
    });

    this.getEvent();
};

PurpleAssistant.prototype.eventSuccess = function(response) {
    Mojo.Log.info("eventSuccess: " + JSON.stringify(response));
    if ("credentials" in response)
    {
        var result = {
            template: this.template,
            username: response.username || this.usernameModel.value,
            config: this.prefs,
            defaultResult: {
                result: {
                    returnValue: true,
                    credentials: response.credentials
                }
            }
        };

        this.controller.stageController.popScene(result);
    }
    else
    {
        this.showPopup(response);
    }
};

PurpleAssistant.prototype.eventFail = function(response) {
    Mojo.Log.error("eventFail: " + JSON.stringify(response));
    if (response.errorCode) {
        var text = response.errorText || response.error || "";
        Mojo.Log.info("Showing error " + text);
        Mojo.Controller.errorDialog(text);
    }
    this.enableControls();
};

PurpleAssistant.prototype.enableControls = function(val) {
    this.controller.get('CreateAccountButton').mojo.deactivate();
    this.toggleControls(true);
};

PurpleAssistant.prototype.disableControls = function(val) {
    this.toggleControls(false);
};

PurpleAssistant.prototype.toggleControls = function(val) {
    val = !val;
    this.usernameModel.disabled = val;
    this.passwordModel.disabled = val;

    this.controller.modelChanged(this.usernameModel);
    this.controller.modelChanged(this.passwordModel);

    for (var name in this.optionsModel)
    {
        this.optionsModel[name].disabled = val;
    }
    this.controller.modelChanged(this.optionsModel)
};

PurpleAssistant.prototype.showPopup = function(popup) {
    Mojo.Log.info("Showing popup " + JSON.stringify(popup));
    var dialog = {
        message: ""
    };
    if ("title" in popup)
        dialog.title = popup.title;
    if ("primary" in popup)
        dialog.message += popup.primary;
    if ("secondary" in popup)
    {
        if (dialog.message != "")
            dialog.message += "<br/>";
        dialog.message += popup.secondary;
        dialog.allowHTMLMessage = true;
    }

    if ("actions" in popup)
    {
        var choices = [];
        for (var i = 0; i<popup.actions.length; i++)
        {
            choices.push({
                label: popup.actions[i],
                value: {
                    id: i,
                    request_id: popup.id
                }
            });
        }

        dialog.choices = choices;
        dialog.preventCancel = true;

        dialog.onChoose = function(value) {
            new Mojo.Service.Request(_ValidatorAddress + "answerEvent", {
                parameters: {
                    answer: value.id,
                    id: value.request_id
                },
                onResponse: function() {
                    this.getEvent();
                }
            });
        };

    }
    else if ("type" in popup)
    {
    }

    this.controller.showAlertDialog(dialog);
};

PurpleAssistant.prototype.getEvent = function() {
    this.controller.serviceRequest(_ValidatorAddress + "getEvent", {
        parameters: {},
        onSuccess: this.eventSuccess.bind(this),
        onFailure: this.eventFail.bind(this),
        onError: this.eventFail.bind(this)
    });
};

var _TypeToTemplate = {
    "string": "textfield",
    "int": "textfield",
    "bool": "checkbox",
    "list": "list"
};

PurpleAssistant.prototype.createOptionsWidget = function(options) {
    for (var name in options) {
        var node = options[name];
        if (node.type in _TypeToTemplate)
        {
            var model = {
                itemId: name,
                name: "optionWidget_" + node.type,
                itemText: node.text,
                disabled: false
            };

            model.element = Mojo.View.render({
                            template: "../templates/" + _TypeToTemplate[node.type],
                            object: model
            });

            if ("default_value" in node)
                model.value = node.default_value || "";

            if (node.type === "list")
            {
                model.choices = [];
                for (var name_ in node.choices)
                    model.choices.push({
                        label: node.choices[name_],
                        value: name_
                    });
            }

            this.optionsModel.items.push(model);
        }
    }

    // Setup widgets
    this.controller.setupWidget("optionWidget_string", {
        textCase: Mojo.Widget.steModeLowerCase
        }, {});

    this.controller.setupWidget("optionWidget_int", {
        charsAllow: function(c) {
            if (/[0-9]/.test(c))
                return true;
            else
                return false;
            }
        }, {});

    this.controller.setupWidget("optionWidget_bool", {}, {});
    this.controller.setupWidget("optionWidget_list", {}, {});

    Mojo.Log.info(JSON.stringify(this.optionsModel));
    this.controller.modelChanged(this.optionsModel);
    this.controller.instantiateChildWidgets(this.controller.get("OptionsList"));

    for (var name in options) {
        if (this.controller.get(name))
        {
            Mojo.Log.info("Found elem " + name + ", setting up listener.");
            this.controller.listen(
                this.controller.get(name),
                Mojo.Event.propertyChange,
                this.prefsChanged.bindAsEventListener(this, name)
            );
        }
    }
};

PurpleAssistant.prototype.prefsChanged = function(ev, name) {
    Mojo.Log.info("Changed pref: " + name + " to " + ev.value);
    this.prefs[name] = ev.value;
};
