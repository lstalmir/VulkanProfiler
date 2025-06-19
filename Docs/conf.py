# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

import sys
import os.path
sys.path.append(os.path.dirname(os.path.realpath(__file__)))

project = 'VulkanProfiler'
copyright = '2025, Łukasz Stalmirski'
author = 'Łukasz Stalmirski'

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = ['sphinx.ext.autosectionlabel', 'sphinx_tabs.tabs', 'sphinx_multiversion', 'vkprof']
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']

# -- Options for versioning --------------------------------------------------
smv_tag_whitelist = r'^release-.*$'
smv_branch_whitelist = r'^master|docs$'
smv_remote_whitelist = 'origin'
smv_outputdir_format = '{ref.name}'
smv_prefer_remote_refs = False

# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = 'sphinx_rtd_theme'
html_style = 'css/style.css'
html_static_path = ['static']
templates_path = ['templates']
