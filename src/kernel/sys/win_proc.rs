/// Defines the system processes that are available to the kernel.
use crate::prelude::*;

#[derive(Debug, Default)]
pub struct Window {
    pub(crate) title: String,
    pub(crate) width: u32,
    pub(crate) height: u32,
}

impl lua::UserData for Window {
    fn add_fields<'lua, F: mlua::UserDataFields<'lua, Self>>(fields: &mut F) {
        fields.add_field_method_get("height", |_, this| Ok(this.height));
        fields.add_field_method_set("height", |_, this, val| {
            this.height = val;
            Ok(())
        });
        fields.add_field_method_get("width", |_, this| Ok(this.width));
        fields.add_field_method_set("width", |_, this, val| {
            this.width = val;
            Ok(())
        });

        fields.add_field_method_get("title", |_, this| Ok(this.title.clone()));
        fields.add_field_method_set("title", |_, this, val| {
            this.title = val;
            Ok(())
        });
    }

    fn add_methods<'lua, M: mlua::UserDataMethods<'lua, Self>>(methods: &mut M) {
        methods.add_method("area", |_, this, ()| Ok(this.width * this.height));
        methods.add_method("diagonal", |_, this, ()| {
            Ok((this.width.pow(2) as f64 + this.height.pow(2) as f64).sqrt())
        });

        // Constructor
        methods.add_meta_function(lua::MetaMethod::Call, |_, ()| Ok(Window::default()));
    }
}